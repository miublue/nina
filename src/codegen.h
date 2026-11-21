#ifndef CODEGEN_H
#define CODEGEN_H

#include "analyzer.h"
#include "lexer.h"
#include "parser.h"

struct codegen {
    struct expr_block ast;
    struct analyzer *analyzer;
    char *out_path;
};

void generate(struct codegen *gen);

#endif
