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
        if (!strcmp(var.name, "_va_list")) EMIT("__nina_value _va_list = _;\n");
        else EMIT("__nina_value %s = __NINA_FROM_INTEGER(0);\n", var.name);
    }
    _generate_expr(gen, func.body);
    EMIT(";\n__nina_return:\nreturn __nina_result;\n}\n");
}

static void _generate_literal(struct codegen *gen, struct token literal)
{
    (void)gen;
    const char *type_str[] = { "__NINA_FROM_INTEGER(%s)", "__NINA_FROM_STRING(strdup(%s))", };
    const int is_str = literal.kind == TK_STRING;
    EMIT(type_str[is_str], literal.data);
}

static void _generate_call_func(struct codegen *gen, struct expr_call_func func)
{
    struct function *func_data = get_function(gen->analyzer, func.name);
    if (!func_data) ERROR("_generate_call_func: function '%s' is not defined. must be a bug in the analyzer\n", func.name);
    EMIT("%s(", func.name);
    for (int i = 0; i < func.num_args; ++i) {
        if (i > 0) EMIT(", ");
        if (func_data->is_variadic && i < func_data->arity && !strcmp(func_data->args[i].name, "_")) {
            EMIT("__nina_make_array(%d, ", func.num_args-i);
        }
        _generate_expr(gen, func.args[i]);
    }
    if (func_data->is_variadic) EMIT(")");
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

static void _generate_ifelse(struct codegen *gen, struct expr_ifelse cond, int is_while)
{
    EMIT("%s (__nina_truthy(", is_while? "while" : "if");
    _generate_expr(gen, cond.cond);
    EMIT(").i == __NINA_TRUE)");
    _generate_expr(gen, cond.body);
    if (cond.elze && !is_while) {
        EMIT("else ");
        _generate_expr(gen, cond.elze);
    }
}

static void _generate_array_index(struct codegen *gen, struct expr_array_index array)
{
    EMIT("__nina_%cet_array(", array.expr? 's' : 'g');
    _generate_expr(gen, array.array);
    EMIT(", ");
    _generate_expr(gen, array.index);
    if (array.expr) {
        EMIT(", ");
        _generate_expr(gen, array.expr);
    }
    EMIT(")");
}

static void _generate_make_array(struct codegen *gen, struct expr_block array)
{
    EMIT("__nina_make_array(%d", array.size);
    for (int i = 0; i < array.size; ++i) {
        EMIT(", ");
        _generate_expr(gen, array.body[i]);
    }
    EMIT(")");
}

static void _generate_return(struct codegen *gen, struct expr_return ret)
{
    const char *rets[] = { "break", "continue", "return" };
    EMIT("%s", rets[ret.ret_type]);
    if (ret.ret_type == RET_RETURN) {
        EMIT(" ");
        if (ret.expr) _generate_expr(gen, ret.expr);
        else EMIT("__NINA_FROM_INTEGER(0)");
    }
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
    case EXPR_SET_ARRAY: case EXPR_GET_ARRAY:
        _generate_array_index(gen, expr->array_index);
        break;
    case EXPR_MAKE_ARRAY:
        _generate_make_array(gen, expr->make_array);
        break;
    case EXPR_BLOCK:
        EMIT("{\n");
        for (int i = 0; i < expr->block.size; ++i) {
            if (expr->block.body[i]->kind < EXPR_BLOCK)
                EMIT("__nina_result = ");
            _generate_expr(gen, expr->block.body[i]);
            EMIT(";\n");
        }
        EMIT("}\n");
        break;
    case EXPR_BINARY:
        _generate_binary(gen, expr->binary);
        break;
    case EXPR_IFELSE:
        _generate_ifelse(gen, expr->cond, 0);
        break;
    case EXPR_WHILE:
        _generate_ifelse(gen, expr->cond, 1);
        break;
    case EXPR_RETURN:
        _generate_return(gen, expr->ret);
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

    EMIT("#include \"std/nina_runtime.h\"\n");
    for (int i = 0; i < gen->ast.size; ++i)
        _generate_toplevel(gen, gen->ast.body[i]);

    EMIT("int main(void) { __nina_value v=__nina_main(); return __NINA_TO_INTEGER(v); }\n");
    fclose(out);
}

