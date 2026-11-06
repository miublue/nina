#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "analyzer.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "module.h"

static void _usage(const char *prg, int err)
{
    printf("usage: %s [-hk] [-o name] input\n", prg);
    exit(err);
}

int main(int argc, char **argv)
{
    char *input_file = NULL, *output_file = "a.out", *gen_file = "__tmp_output.c";
    char cmd[1024] = {0};
    int keep_temp = 0;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-o")) output_file = argv[++i];
        else if (!strcmp(argv[i], "-k")) keep_temp = 1;
        else if (!strcmp(argv[i], "-h")) _usage(argv[0], 0);
        else input_file = argv[i];
    }

    if (!input_file) _usage(argv[0], 1);

    struct parser par = {0};
    par.tokenizer.loc = (struct location) { .file = input_file, .line = 1 };
    if (!(par.tokenizer.text = read_file(input_file))) ERROR("failed to read file '%s'\n", input_file);

    add_include_path("./");
    add_include_path("/usr/lib/nina/");
    add_include_path("/usr/local/lib/nina/");

    par.tokenizer.text_size = strlen(par.tokenizer.text);
    tokenize(&par.tokenizer);

    parse(&par);

    struct analyzer anal = {0};
    analyze(&anal, par.ast);

    struct codegen gen = {
        .ast = par.ast,
        .analyzer = &anal,
        .out_path = gen_file,
    };

    generate(&gen);
    snprintf(cmd, sizeof(cmd), "${NINA_CC:-cc} %s -o %s -lc -lgc -I/usr/lib/nina -I/usr/local/lib/nina", gen_file, output_file);
    system(cmd);
    if (!keep_temp) unlink(gen_file);
    return 0;
}
