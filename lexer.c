#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

static const char *KEYWORDS[] = {
    "extern", "do", "end",
};

static const char *OPERATORS[] = {
    ",", ".", ";", "+", "-", "*", "/", "%", "&", "|", "^", "(", ")", "[", "]",
    "=", "!", "<", ">", "==", "!=", "<=", ">=", "&&", "||", "<<", ">>",
};

static void _append_token(struct tokenizer *tok, struct token token)
{
    if (!tok->max_tokens) tok->tokens = malloc(sizeof(struct token) * (tok->max_tokens = 1024));
    else if (tok->num_tokens >= tok->max_tokens)
        tok->tokens = realloc(tok->tokens, sizeof(struct token) * (tok->max_tokens *= 2));
    tok->tokens[tok->num_tokens++] = token;
}

#define BOUND(T) ((T)->cur < (T)->text_size)
#define CUR(T) ((T)->text[(T)->cur])
#define SEE(T, N) ((T)->text[(T)->cur + (N)])
#define INC(T, N) ((T)->cur += (N))

static void _skip_space(struct tokenizer *tok)
{
    while (BOUND(tok) && isspace(CUR(tok))) {
        if (CUR(tok) == '\n') ++tok->loc.line;
        INC(tok, 1);
    }
    if (BOUND(tok) && CUR(tok) == '#') {
        if (SEE(tok, 1) == '[') {
            while (BOUND(tok)) {
                if (CUR(tok) == '\n') ++tok->loc.line;
                INC(tok, 1);
                if (CUR(tok) == ']' && SEE(tok, 1) == '#') {
                    INC(tok, 2);
                    break;
                }
            }
        } else {
            while (BOUND(tok) && CUR(tok) != '\n') INC(tok, 1);
        }
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

static int _is_operator(int sz, char *ptr)
{
    for (int i = 0; i < (int)(sizeof(OPERATORS)/sizeof(OPERATORS[0])); ++i) {
        if ((int)strlen(OPERATORS[i]) == sz && !strncmp(ptr, OPERATORS[i], sz))
            return i;
    }
    return -1;
}

static struct token _next_token(struct tokenizer *tok)
{
    _skip_space(tok);
    struct token tk = { .loc = tok->loc, .kind = TK_EOF, .data = &CUR(tok) };
    int size = 0;
    if (!BOUND(tok)) return tk;
    if (strchr("\"\'", CUR(tok))) {
        char match = CUR(tok), is_closed = 0;
        tk.kind = TK_STRING;
        // XXX: character escaping
        do ++size, INC(tok, 1); while (CUR(tok) != match && (is_closed = (BOUND(tok) && CUR(tok) != '\n')));
        if (!is_closed) error_loc(tk.loc, "unclosed string\n");
        ++size, INC(tok, 1);
        goto ret;
    }
    if (isdigit(CUR(tok))) {
        tk.kind = TK_NUMBER;
        do ++size, INC(tok, 1); while (isdigit(CUR(tok)));
        goto ret;
    }
    int idx = _is_operator(2, tk.data);
    if (idx == -1) idx = _is_operator(1, tk.data);
    if (idx != -1) {
        tk.kind = TK_COMMA+idx, size = idx >= TK_EQEQ? 2 : 1;
        INC(tok, size);
        goto ret;
    }
    while (!isspace(CUR(tok)) && !strchr("\"\'#", CUR(tok)) && _is_operator(1, &CUR(tok)) == -1)
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

const char *readable_token_kind(char kind)
{
    static const char *HUMAN_READABLE_TOKEN_KIND[] = { "<EOF>", "number", "string", "identifier" };
    if (kind >= TK_EXTERN && kind <= TK_END) return KEYWORDS[kind-TK_EXTERN];
    if (kind >= TK_COMMA && kind <= TK_SHR) return OPERATORS[kind-TK_COMMA];
    return HUMAN_READABLE_TOKEN_KIND[(int)kind];
}

void error_loc(struct location loc, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s:%d: error: ", loc.file, loc.line);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

#undef BOUND
#undef CUR
#undef SEE
#undef INC
