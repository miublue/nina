#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analyzer.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"

static FILE *out;

#define EMIT(...) fprintf(out, __VA_ARGS__)


static const char *binary_operators[] = {
    [TK_ADD]  = "__NINA_BINOP_ADD",
    [TK_SUB]  = "__NINA_BINOP_SUB",
    [TK_MUL]  = "__NINA_BINOP_MUL",
    [TK_DIV]  = "__NINA_BINOP_DIV",
    [TK_MOD]  = "__NINA_BINOP_MOD",
    [TK_LT]   = "__NINA_BINOP_LT",
    [TK_GT]   = "__NINA_BINOP_GT",
    [TK_LEQ]  = "__NINA_BINOP_LE",
    [TK_GEQ]  = "__NINA_BINOP_GE",
    [TK_EQEQ] = "__NINA_BINOP_EQ",
    [TK_NEQ]  = "__NINA_BINOP_NE",
    [TK_AND]  = "__NINA_BINOP_AND",
    [TK_OR]   = "__NINA_BINOP_OR",
    [TK_XOR]  = "__NINA_BINOP_XOR",
    [TK_SHL]  = "__NINA_BINOP_SHL",
    [TK_SHR]  = "__NINA_BINOP_SHR",
    [TK_BAND] = "__NINA_BINOP_BAND",
    [TK_BOR]  = "__NINA_BINOP_BOR",
};

static void _generate_expr(struct codegen *gen, struct expr *expr);

static int _is_variable_arg(struct function *func, char *varname)
{
    for (int i = 0; i < func->arity; ++i) {
        if (!strcmp(func->args[i].name, varname))
            return 1;
    }
    return 0;
}

static void _generate_func(struct codegen *gen, struct expr_define_func func)
{
    char *name = strcmp(func.name, "main") == 0? "__nina_main" : func.name;
    EMIT("%s__nina_value %s(", func.is_extern? "extern " : "", name);
    for (int i = 0; i < func.num_args; ++i) {
        if (i > 0) EMIT(", ");
        EMIT("__nina_value %s", func.args[i].name);
    }
    if (!func.body) {
        EMIT(");\n");
        return;
    }
    EMIT(") {\n__nina_value __nina_result = __NINA_FROM_INTEGER(0);\n");
    struct function *func_data = get_function(gen->analyzer, func.name);
    if (!func_data) ERROR("_generate_func: function '%s' is not defined. must be a bug in the analyzer\n", func.name);
    for (int i = 0; i < func_data->num_vars; ++i) {
        struct func_arg var = func_data->vars[i];
        if (_is_variable_arg(func_data, var.name)) continue;
        EMIT("__nina_value %s = __NINA_FROM_INTEGER(0);\n", var.name);
    }
    _generate_expr(gen, func.body);
    EMIT(";\n__nina_return:\nreturn __nina_result;\n}\n");
}

static void _generate_literal(struct codegen *gen, struct token literal)
{
    (void)gen;
    const char *type_str[] = { "__NINA_FROM_INTEGER(%s)", "__NINA_FROM_STRING(strdup(%s))", };
    EMIT(type_str[literal.kind == TK_STRING], literal.data);
}

static void _generate_call_func(struct codegen *gen, struct expr_call_func func)
{
    EMIT("%s(", func.name);
    for (int i = 0; i < func.num_args; ++i) {
        if (i > 0) EMIT(", ");
        _generate_expr(gen, func.args[i]);
    }
    EMIT(")");
}

static void _generate_binary(struct codegen *gen, struct expr_binary binary)
{
    EMIT("__nina_binary(%s, ", binary_operators[binary.op]);
    _generate_expr(gen, binary.left);
    EMIT(", ");
    _generate_expr(gen, binary.right);
    EMIT(")");
}

static void _generate_expr(struct codegen *gen, struct expr *expr)
{
    switch (expr->kind) {
    case EXPR_LITERAL:
        _generate_literal(gen, expr->literal);
        break;
    case EXPR_CALL_FUNC:
        _generate_call_func(gen, expr->call_func);
        break;
    case EXPR_SET_VAR:
        EMIT("%s = ", expr->set_var.name);
        _generate_expr(gen, expr->set_var.expr);
        break;
    case EXPR_GET_VAR:
        EMIT("(%s)", expr->set_var.name);
        break;
    case EXPR_BLOCK:
        EMIT("{\n");
        for (int i = 0; i < expr->block.size; ++i) {
            EMIT("__nina_result = ");
            _generate_expr(gen, expr->block.body[i]);
            EMIT(";\n");
        }
        EMIT("}\n");
        break;
    case EXPR_BINARY:
        _generate_binary(gen, expr->binary);
        break;
    default: ERROR("_generate_expr: unexpected statement, must be a bug in the analyzer\n");
    }
}

static void _generate_toplevel(struct codegen *gen, struct expr *expr)
{
    switch (expr->kind) {
    case EXPR_DEFINE_FUNC:
        _generate_func(gen, expr->define_func);
        break;
    default: ERROR("_generate_toplevel: unexpected statement, must be a bug in the analyzer\n");
    }
}

void generate(struct codegen *gen)
{
    if (!(out = fopen(gen->out_path, "w+")))
        ERROR("could not open '%s' for writing\n", gen->out_path);

    EMIT("#include \"nina_runtime.h\"\n");
    for (int i = 0; i < gen->ast.size; ++i)
        _generate_toplevel(gen, gen->ast.body[i]);

    EMIT("int main(void) { __nina_value v=__nina_main(); return __NINA_TO_INTEGER(v); }\n");
    fclose(out);
}

