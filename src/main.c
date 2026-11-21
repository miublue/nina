#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "analyzer.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "module.h"

#define VERSION "v0.0.1"

static void _usage(const char *prg, int err)
{
    printf("usage: %s [-hkv] [-o name] files...\n", prg);
    exit(err);
}

int main(int argc, char **argv)
{
    char *input_files[1024], cmd[1024], *output_file = "a.out", *gen_file = "__tmp_output.c";
    int keep_temp = 0, num_files = 0;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-o")) output_file = argv[++i];
        else if (!strcmp(argv[i], "-k")) keep_temp = 1;
        else if (!strcmp(argv[i], "-h")) _usage(argv[0], 0);
        else if (!strcmp(argv[i], "-v")) { printf("%s %s\n", argv[0], VERSION); return 0; }
        else input_files[num_files++] = argv[i];
    }

    if (!num_files) _usage(argv[0], 1);

    add_include_path("./");
    add_include_path("/usr/lib/nina/");
    add_include_path("/usr/local/lib/nina/");

    struct parser par = {0};
    for (int i = 0; i < num_files; ++i) {
        char path[4097];
        if (!realpath(input_files[i], path)) ERROR("could not find file '%s'\n", input_files[i]);
        par.tokenizer.loc = (struct location) { .file = path, .line = 1 };
        if (!(par.tokenizer.text = read_file(path))) ERROR("failed to read file '%s'\n", path);
        par.tokenizer.text_size = strlen(par.tokenizer.text);
        add_imported_file(path);
        tokenize(&par.tokenizer);
        /* remove trailing EOF token */
        if (i+1 < num_files) --par.tokenizer.num_tokens;
    }
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
