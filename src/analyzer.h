#ifndef ANALYZER_H
#define ANALYZER_H

#include "parser.h"

struct function {
    // XXX: return type and whatnot
    char *name;
    int arity, is_extern, is_variadic, num_vars, max_vars;
    struct func_arg args[MAX_FUNC_ARGS];
    struct func_arg *vars;
};

struct class {
    char *name;
    int num_fields, max_fields, num_methods, max_methods;
    struct func_arg *fields;
    struct function *methods;
};

struct analyzer {
    int num_funcs, max_funcs, num_classes, max_classes;
    struct function **funcs, *func;
    struct class **classes;
};

void analyze(struct analyzer *anal, struct expr_block ast);
struct function *get_function(struct analyzer *anal, char *name);
struct func_arg *get_variable(struct function *func, char *name);
struct class *get_class(struct analyzer *anal, char *name);

#endif
