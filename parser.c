#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"
#include "module.h"

#define BOUND(P) (CUR(P) < (P)->tokenizer.num_tokens)
#define CUR(P) ((P)->tokenizer.cur)
#define TOK(P) ((P)->tokenizer.tokens[CUR(P)])
#define INC(P, N) (CUR(P) += (N))

static struct expr *_parse_statement(struct parser *par);
static struct expr *_parse_expression(struct parser *par);
static struct expr *_parse_toplevel(struct parser *par);

static struct expr *_alloc_expr(struct location loc, int kind)
{
    struct expr *expr = calloc(1, sizeof(*expr));
    if (!expr) ERROR("_alloc_expr: buy more ram\n");
    expr->loc = loc, expr->kind = kind;
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
    if (tok.kind != kind) error_loc(tok.loc, "expected '%s', got '%s'\n", readable_token_kind(kind), tok.data);
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
    struct expr *expr = _alloc_expr(TOK(par).loc, EXPR_DEFINE_FUNC);
    if (is_extern) _expect_kind(par, TK_EXTERN, 1);
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
    struct expr *expr = _alloc_expr(TOK(par).loc, EXPR_CALL_FUNC);
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

static struct expr *_parse_make_array(struct parser *par)
{
    struct expr *expr = _alloc_expr(TOK(par).loc, EXPR_MAKE_ARRAY);
    do {
        INC(par, 1);
        if (!BOUND(par) || TOK(par).kind == TK_RSQUARE) break;
        _append_expr(&expr->make_array, _parse_expression(par));
    } while (TOK(par).kind == TK_COMMA);
    _expect_kind(par, TK_RSQUARE, 1);
    return expr;
}

static struct expr *_parse_literal(struct parser *par)
{
    if (TOK(par).kind == TK_LSQUARE) return _parse_make_array(par);
    struct expr *expr = _alloc_expr(TOK(par).loc, EXPR_LITERAL);
    if (TOK(par).kind != TK_NUMBER && TOK(par).kind != TK_STRING)
        error_loc(TOK(par).loc, "unexpected '%s'\n", TOK(par).data);
    expr->literal = TOK(par);
    INC(par, 1);
    return expr;
}

enum { BLOCK_BLOCK, BLOCK_IF, BLOCK_ELSE, BLOCK_WHILE };
static struct expr *_parse_block(struct parser *par, int block_type)
{
    struct expr *expr = _alloc_expr(TOK(par).loc, EXPR_BLOCK);
    if (block_type == BLOCK_BLOCK) _expect_kind(par, TK_DO, 1);
    while (BOUND(par) && TOK(par).kind != TK_END) {
        if (TOK(par).kind == TK_EOF) error_loc(expr->loc, "missing 'end' for 'do' block\n");
        if (TOK(par).kind == TK_ELSE && block_type == BLOCK_IF) return expr;
        _append_expr(&expr->block, _parse_statement(par));
    }
    _expect_kind(par, TK_END, 1);
    if (TOK(par).kind == TK_ELSE) error_loc(TOK(par).loc, "unexpected 'else'\n");
    return expr;
}

static struct expr *_parse_ifelse(struct parser *par)
{
    struct expr *expr = _alloc_expr(TOK(par).loc, EXPR_IFELSE);
    _expect_kind(par, TK_IF, 1);
    expr->cond.cond = _parse_expression(par);
    expr->cond.body = _parse_block(par, BLOCK_IF);
    if (TOK(par).kind == TK_ELSE) {
        INC(par, 1);
        if (TOK(par).kind == TK_IF) expr->cond.elze = _parse_ifelse(par);
        else expr->cond.elze = _parse_block(par, BLOCK_ELSE);
    }
    return expr;
}

static struct expr *_parse_while(struct parser *par)
{
    struct expr *expr = _alloc_expr(TOK(par).loc, EXPR_WHILE);
    _expect_kind(par, TK_WHILE, 1);
    expr->cond.cond = _parse_expression(par);
    expr->cond.body = _parse_block(par, BLOCK_WHILE);
    return expr;
}

static struct expr *_parse_return(struct parser *par, int ret_type)
{
    struct expr *expr = _alloc_expr(TOK(par).loc, EXPR_RETURN);
    INC(par, 1);
    expr->ret.ret_type = ret_type;
    if (ret_type == RET_RETURN && TOK(par).loc.line == expr->loc.line && TOK(par).kind != TK_END)
        expr->ret.expr = _parse_expression(par);
    return expr;
}

static struct expr *_parse_variable(struct parser *par)
{
    char *name = _expect_kind(par, TK_IDENT, 1).data;
    if (TOK(par).kind == TK_LPAREN) return _parse_call_func(par, name);
    const int is_assign = TOK(par).kind == TK_EQ;
    struct expr *expr = _alloc_expr(TOK(par).loc, is_assign? EXPR_SET_VAR : EXPR_GET_VAR);
    expr->set_var.name = name, expr->set_var.expr = NULL;
    if (is_assign) {
        INC(par, 1);
        expr->set_var.expr = _parse_expression(par);
    }
    return expr;
}

static struct expr *_parse_array_index(struct parser *par, struct expr *array)
{
    _expect_kind(par, TK_LSQUARE, 1);
    struct expr *expr = _alloc_expr(TOK(par).loc, EXPR_GET_ARRAY);
    expr->array_index.array = array;
    expr->array_index.index = _parse_expression(par);
    _expect_kind(par, TK_RSQUARE, 1);
    if (TOK(par).kind == TK_EQ) {
        INC(par, 1);
        expr->kind = EXPR_SET_ARRAY, expr->array_index.expr = _parse_expression(par);
    }
    return expr;
}

static struct expr *_parse_term(struct parser *par)
{
    struct expr *expr = NULL;
    if (TOK(par).kind == TK_EOF) return expr;
    switch (TOK(par).kind) {
    case TK_NUMBER: case TK_STRING: case TK_LSQUARE:
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
    default: error_loc(TOK(par).loc, "unexpected '%s'\n", TOK(par).data);
    }
    return expr;
}

static struct expr *_parse_factor(struct parser *par)
{
    struct expr *expr = _parse_term(par);
    while (TOK(par).kind == TK_LSQUARE) expr = _parse_array_index(par, expr);
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
    struct expr *left = _parse_factor(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const struct token tok = TOK(par);
        INC(par, 1);
        struct expr *expr = _alloc_expr(tok.loc, EXPR_BINARY);
        expr->binary.op = tok.kind, expr->binary.left = left, expr->binary.right = _parse_factor(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_andor(struct parser *par)
{
    const int toks[] = { TK_AND, TK_OR, TK_XOR };
    struct expr *left = _parse_bitshifts(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const struct token tok = TOK(par);
        INC(par, 1);
        struct expr *expr = _alloc_expr(tok.loc, EXPR_BINARY);
        expr->binary.op = tok.kind, expr->binary.left = left, expr->binary.right = _parse_bitshifts(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_muldiv(struct parser *par)
{
    const int toks[] = { TK_MUL, TK_DIV, TK_MOD };
    struct expr *left = _parse_andor(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const struct token tok = TOK(par);
        INC(par, 1);
        struct expr *expr = _alloc_expr(tok.loc, EXPR_BINARY);
        expr->binary.op = tok.kind, expr->binary.left = left, expr->binary.right = _parse_andor(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_addsub(struct parser *par)
{
    const int toks[] = { TK_ADD, TK_SUB };
    struct expr *left = _parse_muldiv(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const struct token tok = TOK(par);
        INC(par, 1);
        struct expr *expr = _alloc_expr(tok.loc, EXPR_BINARY);
        expr->binary.op = tok.kind, expr->binary.left = left, expr->binary.right = _parse_muldiv(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_comparison(struct parser *par)
{
    const int toks[] = { TK_EQEQ, TK_NEQ, TK_LT, TK_GT, TK_LEQ, TK_GEQ };
    struct expr *left = _parse_addsub(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const struct token tok = TOK(par);
        INC(par, 1);
        struct expr *expr = _alloc_expr(tok.loc, EXPR_BINARY);
        expr->binary.op = tok.kind, expr->binary.left = left, expr->binary.right = _parse_addsub(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_expression(struct parser *par)
{
    const int toks[] = { TK_BAND, TK_BOR };
    struct expr *left = _parse_comparison(par);
    while (_is_token_type(toks, (int)(sizeof(toks)/sizeof(toks[0])), TOK(par).kind)) {
        const struct token tok = TOK(par);
        INC(par, 1);
        struct expr *expr = _alloc_expr(tok.loc, EXPR_BINARY);
        expr->binary.op = tok.kind, expr->binary.left = left, expr->binary.right = _parse_comparison(par);
        left = expr;
    }
    return left;
}

static struct expr *_parse_statement(struct parser *par)
{
    struct expr *expr = NULL;
    if (TOK(par).kind == TK_EOF) return expr;
    switch (TOK(par).kind) {
    case TK_DO: expr = _parse_block(par, BLOCK_BLOCK); break;
    case TK_IF: expr = _parse_ifelse(par); break;
    case TK_WHILE: expr = _parse_while(par); break;
    case TK_BREAK: case TK_CONTINUE: case TK_RETURN:
        expr = _parse_return(par, TOK(par).kind-TK_BREAK); break;
    default: expr = _parse_expression(par); break;
    }
    return expr;
}

static void _parse_import_module(struct parser *par)
{
    const struct location loc = TOK(par).loc;
    do {
        char path[4096] = {0}, lookup_path[4097] = {0};
        do {
            INC(par, 1);
            if (strlen(path)) strcat(path, "/");
            strcat(path, TOK(par).data);
            INC(par, 1);
        } while(TOK(par).kind == TK_DOT);
        strcat(path, ".ni");
        struct parser p = {0};
        p.tokenizer.loc = (struct location) { .file = path, .line = 1 };

        for (int i = 0; i < module_paths.num_include_paths; ++i) {
            sprintf(lookup_path, "%s/%s", module_paths.include_paths[i], path);
            if ((p.tokenizer.text = read_file(lookup_path))) break;
        }

        if (has_file_been_imported(lookup_path)) continue;
        add_imported_file(lookup_path);
        if (!p.tokenizer.text) error_loc(loc, "could not import module '%s'\n", path);
        p.tokenizer.text_size = strlen(p.tokenizer.text);
        tokenize(&p.tokenizer);
        parse(&p);
        for (int i = 0; i < p.ast.size; ++i) _append_expr(&par->ast, p.ast.body[i]);
    } while (TOK(par).kind == TK_COMMA);
}

static struct expr *_parse_toplevel(struct parser *par)
{
    struct expr *expr = NULL;
    if (TOK(par).kind == TK_EOF) return expr;
    while (TOK(par).kind == TK_IMPORT) _parse_import_module(par);
    switch (TOK(par).kind) {
    case TK_IDENT: case TK_EXTERN:
        expr = _parse_define_func(par, TOK(par).kind == TK_EXTERN);
        break;
    default:
        error_loc(TOK(par).loc, "unexpected '%s'\n", TOK(par).data);
        break;
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
#undef INC
#undef TOk
