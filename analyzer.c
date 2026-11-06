#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analyzer.h"
#include "lexer.h"
#include "parser.h"

static void _analyze_expr(struct analyzer *anal, struct expr *expr);

static void _append_function(struct analyzer *anal, struct function *func)
{
    if (!anal->max_funcs) anal->funcs = calloc(anal->max_funcs = 1024, sizeof(struct function*));
    if (anal->num_funcs >= anal->max_funcs)
        anal->funcs = realloc(anal->funcs, sizeof(struct function*) * (anal->max_funcs *= 2));
    anal->funcs[anal->num_funcs++] = func;
}

static void _append_variable(struct function *func, struct func_arg var)
{
    if (!func->max_vars) func->vars = calloc(func->max_vars = 1024, sizeof(struct func_arg));
    if (func->num_vars >= func->max_vars)
        func->vars = realloc(func->vars, sizeof(struct func_arg) * (func->max_vars *= 2));
    func->vars[func->num_vars++] = var;
}

struct function *get_function(struct analyzer *anal, char *name)
{
    for (int i = 0; i < anal->num_funcs; ++i) {
        if (!strcmp(anal->funcs[i]->name, name))
            return anal->funcs[i];
    }
    return NULL;
}

struct func_arg *get_variable(struct function *func, char *name)
{
    if (!func) return NULL;
    for (int i = 0; i < func->num_vars; ++i) {
        if (!strcmp(func->vars[i].name, name))
            return &func->vars[i];
    }
    return NULL;
}

static void _define_function(struct analyzer *anal, struct expr *expr)
{
    struct function *prev = anal->func, *check = get_function(anal, expr->define_func.name);
    if (check) error_loc(expr->loc, "attempt to redefine function '%s'\n", expr->define_func.name);
    anal->func = calloc(1, sizeof(struct function));
    anal->func->name = expr->define_func.name;
    anal->func->arity = expr->define_func.num_args;
    anal->func->is_extern = expr->define_func.is_extern;
    for (int i = 0; i < anal->func->arity; ++i) {
        anal->func->args[i] = expr->define_func.args[i];
        _append_variable(anal->func, anal->func->args[i]);
    }
    if (expr->define_func.body && expr->define_func.body->kind == EXPR_BLOCK) {
        struct expr_block *block = &expr->define_func.body->block;
        for (int i = 0; i < block->size; ++i) {
            if (block->body[i]->kind == EXPR_SET_VAR) {
                struct func_arg var = { .name = block->body[i]->set_var.name };
                _append_variable(anal->func, var);
            }
        }
    }
    _append_function(anal, anal->func);
    anal->func = prev;
}

static void _analyze_call_func(struct analyzer *anal, struct expr *expr)
{
    // XXX: you'd also type-check all the arguments here
    struct function *func = get_function(anal, expr->call_func.name);
    if (!func) error_loc(expr->loc, "function '%s' does not exist\n", expr->call_func.name);
    if (func->arity != expr->call_func.num_args) {
        error_loc(expr->loc, "function '%s' expects %d argument(s), got %d\n",
                func->name, func->arity, expr->call_func.num_args);
    }

    for (int i = 0; i < expr->call_func.num_args; ++i)
        _analyze_expr(anal, expr->call_func.args[i]);
}

static void _analyze_set_var(struct analyzer *anal, struct expr *expr)
{
    struct func_arg *var = get_variable(anal->func, expr->set_var.name);
    if (!var) error_loc(expr->loc, "variable '%s' does not exist\n", expr->set_var.name);
    if (expr->kind == EXPR_SET_VAR && expr->set_var.expr)
        _analyze_expr(anal, expr->set_var.expr);
}

static void _analyze_expr(struct analyzer *anal, struct expr *expr)
{
    switch (expr->kind) {
    case EXPR_LITERAL: break;
    case EXPR_SET_VAR: case EXPR_GET_VAR:
        _analyze_set_var(anal, expr);
        break;
    case EXPR_CALL_FUNC:
        _analyze_call_func(anal, expr);
        break;
    case EXPR_DEFINE_FUNC: {
        anal->func = get_function(anal, expr->define_func.name);
        if (expr->define_func.body)
            _analyze_expr(anal, expr->define_func.body);
    } break;
    case EXPR_BLOCK:
        for (int i = 0; i < expr->block.size; ++i)
            _analyze_expr(anal, expr->block.body[i]);
        break;
    case EXPR_BINARY:
        _analyze_expr(anal, expr->binary.left);
        _analyze_expr(anal, expr->binary.right);
        break;
    default: error_loc(expr->loc, "unexpected expression\n");
    }
}

void analyze(struct analyzer *anal, struct expr_block ast)
{
    anal->num_funcs = anal->max_funcs = 0;
    anal->func = NULL;
    // first pass, declare all top-level functions and whatnot
    for (int i = 0; i < ast.size; ++i) {
        if (ast.body[i]->kind == EXPR_DEFINE_FUNC)
            _define_function(anal, ast.body[i]);
    }

    // second pass, actually analyze inside the functions
    for (int i = 0; i < ast.size; ++i)
        _analyze_expr(anal, ast.body[i]);

    struct function *main = get_function(anal, "main");
    if (!main) ERROR("missing function 'main'\n");
}
