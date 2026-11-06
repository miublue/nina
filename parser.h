#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// XXX: actual, proper error reporting (with file location and whatnot)
#define ERROR(...) do { fprintf(stderr, "error: "__VA_ARGS__); exit(1); } while(0)
#define MAX_FUNC_ARGS 32

enum {
    EXPR_LITERAL,
    EXPR_CALL_FUNC,
    EXPR_DEFINE_FUNC,
    EXPR_SET_VAR,
    EXPR_GET_VAR,
    EXPR_BINARY,
    EXPR_BLOCK,
};

struct func_arg {
    char *name;
};

struct expr_block {
    int size, capacity;
    struct expr **body;
};

struct expr_call_func {
    char *name;
    int num_args;
    struct expr *args[MAX_FUNC_ARGS];
};

struct expr_define_func {
    char *name;
    int num_args, is_extern;
    struct func_arg args[MAX_FUNC_ARGS];
    struct expr *body;
};

struct expr_set_var {
    char *name;
    struct expr *expr;
};

struct expr_binary {
    int op;
    struct expr *left, *right;
};

struct expr {
    int kind;
    union {
        struct token literal;
        struct expr_call_func call_func;
        struct expr_define_func define_func;
        struct expr_block block;
        struct expr_set_var set_var;
        struct expr_binary binary;
    };
};

struct parser {
    struct tokenizer tokenizer;
    struct expr_block ast;
};

void parse(struct parser *par);

#endif
