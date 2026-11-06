#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

static const char *KEYWORDS[] = {
    "extern", "do", "end",
};

#define CHAR_OPERATORS "=,;()"
// XXX: multi-character operators (eg. == != >>)

static void _append_token(struct tokenizer *tok, struct token token)
{
    if (!tok->max_tokens) tok->tokens = malloc(sizeof(struct token) * (tok->max_tokens = 1024));
    else if (tok->num_tokens >= tok->max_tokens)
        tok->tokens = realloc(tok->tokens, sizeof(struct token) * (tok->max_tokens *= 2));
    tok->tokens[tok->num_tokens++] = token;
}

#define BOUND(T) ((T)->cur < (T)->text_size)
#define CUR(T) ((T)->text[(T)->cur])
#define INC(T, N) ((T)->cur += (N))

static void _skip_space(struct tokenizer *tok)
{
    while (BOUND(tok) && isspace(CUR(tok))) INC(tok, 1);
    if (BOUND(tok) && CUR(tok) == '#') {
        while (BOUND(tok) && CUR(tok) != '\n') INC(tok, 1);
        _skip_space(tok);
    }
}

static int _is_keyword(int sz, char *ptr)
{
    for (int i = 0; i < (int)(sizeof(KEYWORDS)/sizeof(KEYWORDS[0])); ++i) {
        if ((int)strlen(KEYWORDS[i]) == sz && !strncmp(ptr, KEYWORDS[i], sz))
            return i;
    }
    return -1;
}

static struct token _next_token(struct tokenizer *tok)
{
    _skip_space(tok);
    struct token tk = { .kind = TK_EOF, .data = &CUR(tok) };
    int size = 0;
    if (!BOUND(tok)) return tk;
    if (CUR(tok) == '\"') {
        tk.kind = TK_STRING;
        // XXX: character escaping
        do ++size, INC(tok, 1); while (CUR(tok) != '\"');
        ++size, INC(tok, 1);
        goto ret;
    }
    if (isdigit(CUR(tok))) {
        tk.kind = TK_NUMBER;
        do ++size, INC(tok, 1); while (isdigit(CUR(tok)));
        goto ret;
    }
    if (strchr(CHAR_OPERATORS, CUR(tok))) {
        int idx;
        for (idx = 0; idx < (int)strlen(CHAR_OPERATORS); ++idx) {
            if (CUR(tok) == CHAR_OPERATORS[idx]) break;
        }
        tk.kind = TK_EQUALS+idx;
        ++size, INC(tok, 1);
        goto ret;
    }
    while (!isspace(CUR(tok)) && !strchr(CHAR_OPERATORS"\"#", CUR(tok)))
        ++size, INC(tok, 1);
    tk.kind = TK_EXTERN+_is_keyword(size, tk.data);
ret:
    tk.data = strndup(tk.data, size);
    return tk;
}

void tokenize(struct tokenizer *tok)
{
    while (BOUND(tok)) {
        struct token token = _next_token(tok);
        _append_token(tok, token);
        if (token.kind == TK_EOF) break;
    }
}

const char *readable_token_kind(int kind)
{
    static const char *HUMAN_READABLE_TOKEN_KIND[] = {
        "<EOF>", "number", "string", "identifier",
        "extern", "do", "end", "=", ",", ";", "(", ")",
    };

    return HUMAN_READABLE_TOKEN_KIND[kind];
}


#undef BOUND
#undef CUR
#undef INC
