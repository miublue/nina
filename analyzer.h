#ifndef ANALYZER_H
#define ANALYZER_H

#include "parser.h"

struct function {
    // XXX: return type and whatnot
    char *name;
    int arity, is_extern, num_vars, max_vars;
    struct func_arg args[MAX_FUNC_ARGS];
    struct func_arg *vars;
};

struct analyzer {
    int num_funcs, max_funcs;
    struct function **funcs, *func;
};

void analyze(struct analyzer *anal, struct expr_block ast);
struct function *get_function(struct analyzer *anal, char *name);
struct func_arg *get_variable(struct function *func, char *name);

#endif
