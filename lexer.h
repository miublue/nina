#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#define ERROR(...) do { fprintf(stderr, "error: "__VA_ARGS__); exit(1); } while(0)

enum {
    TK_EOF, TK_NUMBER, TK_STRING, TK_IDENT,
    TK_EXTERN, TK_IMPORT, TK_IF, TK_ELSE, TK_WHILE, TK_BREAK, TK_CONTINUE, TK_RETURN,
    TK_DO, TK_END,
    TK_COMMA, TK_DOT, TK_SEMICOLON, TK_ADD, TK_SUB, TK_MUL, TK_DIV, TK_MOD, TK_AND, TK_OR,
    TK_XOR, TK_LPAREN, TK_RPAREN, TK_LSQUARE, TK_RSQUARE, TK_EQ, TK_NOT, TK_LT, TK_GT,
    TK_EQEQ, TK_NEQ, TK_LEQ, TK_GEQ, TK_BAND, TK_BOR, TK_SHL, TK_SHR,
};

struct location {
    unsigned int line;
    const char *file;
};

struct token {
    struct location loc;
    char kind, *data;
};

struct tokenizer {
    int text_size, cur, num_tokens, max_tokens;
    struct location loc;
    struct token *tokens;
    char *text;
};

void tokenize(struct tokenizer *tok);
char *read_file(const char *path);
const char *readable_token_kind(char kind);
void error_loc(struct location loc, const char *fmt, ...);

#endif
