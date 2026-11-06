#ifndef LEXER_H
#define LEXER_H

enum {
    TK_EOF, TK_NUMBER, TK_STRING, TK_IDENT,
    TK_EXTERN, TK_DO, TK_END,
    TK_EQUALS, TK_COMMA, TK_SEMICOLON, TK_LPAREN, TK_RPAREN,
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
