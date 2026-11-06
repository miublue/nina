#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "analyzer.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"

char *read_file(const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    const int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *text = malloc(size+1);
    fread(text, 1, size, file);
    fclose(file);
    return text;
}

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
    if (!(par.tokenizer.text = read_file(input_file))) {
        fprintf(stderr, "error: failed to read file '%s'\n", input_file);
        return 1;
    }

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
    snprintf(cmd, sizeof(cmd), "${CC:-tcc} %s -o %s -lc -lgc", gen_file, output_file);
    system(cmd);
    if (!keep_temp) unlink(gen_file);

    return 0;
}
