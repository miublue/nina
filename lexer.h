#ifndef LEXER_H
#define LEXER_H

enum {
    TK_EOF, TK_NUMBER, TK_STRING, TK_IDENT,
    TK_EXTERN, TK_DO, TK_END,
    TK_COMMA, TK_DOT, TK_SEMICOLON, TK_ADD, TK_SUB, TK_MUL, TK_DIV, TK_MOD, TK_AND, TK_OR,
    TK_XOR, TK_LPAREN, TK_RPAREN, TK_LSQUARE, TK_RSQUARE, TK_EQ, TK_NOT, TK_LT, TK_GT,
    TK_EQEQ, TK_NEQ, TK_LEQ, TK_GEQ, TK_BAND, TK_BOR, TK_SHL, TK_SHR,
};

struct token {
    int kind;
    char *data;
};

struct tokenizer {
    int text_size, cur, num_tokens, max_tokens;
    struct token *tokens;
    char *text;
};

void tokenize(struct tokenizer *tok);
const char *readable_token_kind(int kind);

#endif
