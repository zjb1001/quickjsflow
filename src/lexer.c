#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "quickjsflow/lexer.h"

static int is_ident_start(int c) {
    return (c == '_' || c == '$' || isalpha(c));
}

static int is_ident_part(int c) {
    return (c == '_' || c == '$' || isalnum(c));
}

static char current_char(Lexer *lx) {
    if (lx->pos >= lx->length) return '\0';
    return lx->input[lx->pos];
}

static char peek_char(Lexer *lx) {
    if (lx->pos + 1 >= lx->length) return '\0';
    return lx->input[lx->pos + 1];
}

static void advance(Lexer *lx) {
    if (lx->pos >= lx->length) return;
    char c = lx->input[lx->pos++];
    if (c == '\n') {
        lx->line += 1;
        lx->col = 1;
    } else {
        lx->col += 1;
    }
}

static void skip_whitespace(Lexer *lx) {
    for (;;) {
        char c = current_char(lx);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(lx);
            continue;
        }
        break;
    }
}

static char *substr_dup(const char *src, size_t start, size_t end) {
    if (end <= start) {
        char *s = (char *)malloc(1);
        if (s) s[0] = '\0';
        return s;
    }
    size_t len = end - start;
    char *s = (char *)malloc(len + 1);
    if (!s) return NULL;
    memcpy(s, src + start, len);
    s[len] = '\0';
    return s;
}

static Token make_token(Lexer *lx, TokenType type, int sl, int sc, int el, int ec, size_t s, size_t e) {
    Token t;
    t.type = type;
    t.start_line = sl;
    t.start_col = sc;
    t.end_line = el;
    t.end_col = ec;
    t.lexeme = substr_dup(lx->input, s, e);
    t.error = 0;
    t.error_kind = NULL;
    return t;
}

static Token make_error_token(Lexer *lx, const char *kind, int sl, int sc, int el, int ec, size_t s, size_t e) {
    Token t = make_token(lx, TOKEN_ERROR, sl, sc, el, ec, s, e);
    t.error = 1;
    t.error_kind = kind;
    return t;
}

static Token read_line_comment(Lexer *lx) {
    int sl = lx->line, sc = lx->col;
    size_t start = lx->pos;
    while (current_char(lx) != '\n' && current_char(lx) != '\0') advance(lx);
    size_t end = lx->pos;
    return make_token(lx, TOKEN_COMMENT_LINE, sl, sc, lx->line, lx->col, start, end);
}

static Token read_block_comment(Lexer *lx) {
    int sl = lx->line, sc = lx->col;
    size_t start = lx->pos;
    int closed = 0;
    advance(lx); // '/'
    advance(lx); // '*'
    while (current_char(lx) != '\0') {
        if (current_char(lx) == '*' && peek_char(lx) == '/') {
            advance(lx);
            advance(lx);
            closed = 1;
            break;
        }
        advance(lx);
    }
    size_t end = lx->pos;
    if (!closed) {
        return make_error_token(lx, "UnterminatedBlockComment", sl, sc, lx->line, lx->col, start, end);
    }
    return make_token(lx, TOKEN_COMMENT_BLOCK, sl, sc, lx->line, lx->col, start, end);
}

static Token read_string(Lexer *lx, char quote) {
    int sl = lx->line, sc = lx->col;
    size_t start = lx->pos;
    advance(lx); // opening quote
    int closed = 0;
    while (current_char(lx) != '\0') {
        char c = current_char(lx);
        if (c == '\\') {
            advance(lx);
            if (current_char(lx) != '\0') advance(lx);
            continue;
        }
        if (c == '\n') {
            break;
        }
        if (c == quote) {
            advance(lx);
            closed = 1;
            break;
        }
        advance(lx);
    }
    size_t end = lx->pos;
    if (!closed) {
        return make_error_token(lx, "UnterminatedString", sl, sc, lx->line, lx->col, start, end);
    }
    return make_token(lx, TOKEN_STRING, sl, sc, lx->line, lx->col, start, end);
}

static Token read_number(Lexer *lx) {
    int sl = lx->line, sc = lx->col;
    size_t start = lx->pos;
    int seen_dot = 0;
    while (isdigit(current_char(lx))) advance(lx);
    if (current_char(lx) == '.') {
        seen_dot = 1;
        advance(lx);
        while (isdigit(current_char(lx))) advance(lx);
    }
    if (current_char(lx) == 'e' || current_char(lx) == 'E') {
        advance(lx);
        if (current_char(lx) == '+' || current_char(lx) == '-') advance(lx);
        while (isdigit(current_char(lx))) advance(lx);
    }
    size_t end = lx->pos;
    (void)seen_dot;
    return make_token(lx, TOKEN_NUMBER, sl, sc, lx->line, lx->col, start, end);
}

static Token read_identifier_or_keyword(Lexer *lx) {
    int sl = lx->line, sc = lx->col;
    size_t start = lx->pos;
    advance(lx);
    while (is_ident_part(current_char(lx))) advance(lx);
    size_t end = lx->pos;
    return make_token(lx, TOKEN_IDENTIFIER, sl, sc, lx->line, lx->col, start, end);
}

static Token read_punctuator(Lexer *lx) {
    int sl = lx->line, sc = lx->col;
    size_t start = lx->pos;
    char c = current_char(lx);
    char n = peek_char(lx);
    if (c == '/' && n == '/') {
        return read_line_comment(lx);
    }
    if (c == '/' && n == '*') {
        return read_block_comment(lx);
    }
    // handle a few common two-char punctuators
    if ((c == '=' && n == '=') || (c == '!' && n == '=') || (c == '<' && n == '=') || (c == '>' && n == '=') ||
        (c == '+' && n == '+') || (c == '-' && n == '-') || (c == '&' && n == '&') || (c == '|' && n == '|')) {
        advance(lx);
        advance(lx);
        size_t end2 = lx->pos;
        return make_token(lx, TOKEN_PUNCTUATOR, sl, sc, lx->line, lx->col, start, end2);
    }
    advance(lx);
    size_t end = lx->pos;
    return make_token(lx, TOKEN_PUNCTUATOR, sl, sc, lx->line, lx->col, start, end);
}

void lexer_init(Lexer *lx, const char *input, size_t length) {
    lx->input = input;
    lx->length = length;
    lx->pos = 0;
    lx->line = 1;
    lx->col = 1;
}

Token lexer_next(Lexer *lx) {
    skip_whitespace(lx);
    int sl = lx->line, sc = lx->col;
    char c = current_char(lx);
    if (c == '\0') {
        return make_token(lx, TOKEN_EOF, sl, sc, lx->line, lx->col, lx->pos, lx->pos);
    }
    if (c == '\'' || c == '"') {
        return read_string(lx, c);
    }
    if (is_ident_start(c)) {
        return read_identifier_or_keyword(lx);
    }
    if (isdigit(c)) {
        return read_number(lx);
    }
    return read_punctuator(lx);
}

void token_free(Token *tok) {
    if (tok && tok->lexeme) {
        free(tok->lexeme);
        tok->lexeme = NULL;
    }
}
