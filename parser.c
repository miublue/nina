#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"

#define BOUND(P) (CUR(P) < (P)->tokenizer.num_tokens)
#define CUR(P) ((P)->tokenizer.cur)
#define TOK(P) ((P)->tokenizer.tokens[CUR(P)])
#define INC(P, N) (CUR(P) += (N))

static struct expr *_parse_statement(struct parser *par);
static struct expr *_parse_expression(struct parser *par);
static struct expr *_parse_toplevel(struct parser *par);

static struct expr *_alloc_expr(int kind)
{
    struct expr *expr = calloc(1, sizeof(*expr));
    if (!expr) ERROR("_alloc_expr: buy more ram\n");
    expr->kind = kind;
    return expr;
}

static void _append_expr(struct expr_block *block, struct expr *expr)
{
    if (!block->capacity) block->body = calloc(block->capacity = 1024, sizeof(struct expr*));
    if (block->size >= block->capacity)
        block->body = realloc(block->body, sizeof(struct expr*) * (block->capacity *= 2));
    block->body[block->size++] = expr;
}

static struct token _expect_kind(struct parser *par, int kind, int jump_over)
{
    struct token tok = TOK(par);
    if (tok.kind != kind) ERROR("expected '%s', got '%s'\n", readable_token_kind(kind), tok.data);
    INC(par, jump_over);
    return tok;
}

static struct func_arg _parse_func_arg(struct parser *par)
{
    // XXX: if i ever add static type-checking, i should do some here
    char *name = _expect_kind(par, TK_IDENT, 1).data;
    return (struct func_arg) { .name = name };
}

static struct expr *_parse_define_func(struct parser *par, int is_extern)
{
    struct expr *expr = _alloc_expr(EXPR_DEFINE_FUNC);
    if (is_extern) INC(par, 1);
    expr->define_func.name = _expect_kind(par, TK_IDENT, 1).data;
    expr->define_func.is_extern = is_extern, expr->define_func.num_args = 0;
    do {
        _expect_kind(par, expr->define_func.num_args? TK_COMMA : TK_LPAREN, 1);
        if (TOK(par).kind == TK_RPAREN) break;
        expr->define_func.args[expr->define_func.num_args++] = _parse_func_arg(par);
    } while (TOK(par).kind == TK_COMMA);
    _expect_kind(par, TK_RPAREN, 1);
    if (TOK(par).kind == TK_EQ) {
        INC(par, 1);
        expr->define_func.body = _parse_statement(par);
    } else {
        expr->define_func.body = NULL;
    }
    return expr;
}

static struct expr *_parse_call_func(struct parser *par, char *name)
{
    struct expr *expr = _alloc_expr(EXPR_CALL_FUNC);
    expr->call_func.name = name;
    do {
        _expect_kind(par, expr->call_func.num_args? TK_COMMA : TK_LPAREN, 1);
        if (TOK(par).kind == TK_RPAREN) break;
        struct expr *arg = _parse_expression(par);
        expr->call_func.args[expr->call_func.num_args++] = arg;
    } while (TOK(par).kind == TK_COMMA);
    _expect_kind(par, TK_RPAREN, 1);
    return expr;
}

static struct expr *_parse_literal(struct parser *par)
{
    struct expr *expr = _alloc_expr(EXPR_LITERAL);
    if (TOK(par).kind != TK_NUMBER && TOK(par).kind != TK_STRING)
        ERROR("unexpected '%s'\n", TOK(par).data);
    expr->literal = TOK(par);
    INC(par, 1);
    return expr;
}

static struct expr *_parse_block(struct parser *par)
{
    struct expr *expr = _alloc_expr(EXPR_BLOCK);
    _expect_kind(par, TK_DO, 1);
    while (BOUND(par) && TOK(par).kind != TK_END) {
        _append_expr(&expr->block, _parse_statement(par));
    }
    _expect_kind(par, TK_END, 1);
    return expr;
}

static struct expr *_parse_variable(struct parser *par)
{
    char *name = _expect_kind(par, TK_IDENT, 1).data;
    if (TOK(par).kind == TK_LPAREN) return _parse_call_func(par, name);
    const int is_assign = TOK(par).kind == TK_EQ;
    struct expr *expr = _alloc_expr(is_assign? EXPR_SET_VAR : EXPR_GET_VAR);
    expr->set_var.name = name, expr->set_var.expr = NULL;
    if (is_assign) {
        INC(par, 1);
        expr->set_var.expr = _parse_expression(par);
    }
    return expr;
}

static struct expr *_parse_term(struct parser *par)
{
    struct expr *expr = NULL;
    if (TOK(par).kind == TK_EOF) return expr;
    switch (TOK(par).kind) {
    case TK_NUMBER: case TK_STRING:
        expr = _parse_literal(par);
        break;
    case TK_IDENT:
        expr = _parse_variable(par);
        break;
    case TK_LPAREN:
        INC(par, 1);
        expr = _parse_expression(par);
        _expect_kind(par, TK_RPAREN, 1);
        break;
    default: ERROR("unexpected '%s'\n", TOK(par).data);
    }
    return expr;
}

static int _is_token_type(const int *types, const int len, int type)
{
    for (int i = 0; i < len; ++i)
        if (type == types[i]) return 1;
    return 0;
}

static struct expr *_parse_bitshifts(struct parser *par)
{
    const int toks[] = { TK_SHL, TK_SHR };
    struct expr *left = _parse_term(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const int op = TOK(par).kind;
        INC(par, 1);
        struct expr *expr = _alloc_expr(EXPR_BINARY);
        expr->binary.op = op, expr->binary.left = left, expr->binary.right = _parse_term(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_andor(struct parser *par)
{
    const int toks[] = { TK_AND, TK_OR, TK_XOR };
    struct expr *left = _parse_bitshifts(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const int op = TOK(par).kind;
        INC(par, 1);
        struct expr *expr = _alloc_expr(EXPR_BINARY);
        expr->binary.op = op, expr->binary.left = left, expr->binary.right = _parse_bitshifts(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_muldiv(struct parser *par)
{
    const int toks[] = { TK_MUL, TK_DIV, TK_MOD };
    struct expr *left = _parse_andor(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const int op = TOK(par).kind;
        INC(par, 1);
        struct expr *expr = _alloc_expr(EXPR_BINARY);
        expr->binary.op = op, expr->binary.left = left, expr->binary.right = _parse_andor(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_addsub(struct parser *par)
{
    const int toks[] = { TK_ADD, TK_SUB };
    struct expr *left = _parse_muldiv(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const int op = TOK(par).kind;
        INC(par, 1);
        struct expr *expr = _alloc_expr(EXPR_BINARY);
        expr->binary.op = op, expr->binary.left = left, expr->binary.right = _parse_muldiv(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_comparison(struct parser *par)
{
    const int toks[] = { TK_EQEQ, TK_NEQ, TK_LT, TK_GT, TK_LEQ, TK_GEQ };
    struct expr *left = _parse_addsub(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const int op = TOK(par).kind;
        INC(par, 1);
        struct expr *expr = _alloc_expr(EXPR_BINARY);
        expr->binary.op = op, expr->binary.left = left, expr->binary.right = _parse_addsub(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_expression(struct parser *par)
{
    const int toks[] = { TK_BAND, TK_BOR };
    struct expr *left = _parse_comparison(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const int op = TOK(par).kind;
        INC(par, 1);
        struct expr *expr = _alloc_expr(EXPR_BINARY);
        expr->binary.op = op, expr->binary.left = left, expr->binary.right = _parse_comparison(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_statement(struct parser *par)
{
    struct expr *expr = NULL;
    if (TOK(par).kind == TK_DO) expr = _parse_block(par);
    else expr = _parse_expression(par);
    return expr;
}

static struct expr *_parse_toplevel(struct parser *par)
{
    struct expr *expr = NULL;
    if (TOK(par).kind == TK_EOF) return expr;
    switch (TOK(par).kind) {
    case TK_IDENT: case TK_EXTERN:
        expr = _parse_define_func(par, TOK(par).kind == TK_EXTERN);
        break;
    default: ERROR("unexpected '%s'\n", TOK(par).data);
    }
    return expr;
}

void parse(struct parser *par)
{
    CUR(par) = 0;
    while (BOUND(par)) {
        if (TOK(par).kind == TK_EOF) break;
        struct expr *expr = _parse_toplevel(par);
        if (!expr) break;
        _append_expr(&par->ast, expr);
    }
}

#undef BOUND
#undef CUR
#undef TOk
#undef INC
