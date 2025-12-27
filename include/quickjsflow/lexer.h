#ifndef QUICKJSFLOW_LEXER_H
#define QUICKJSFLOW_LEXER_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_PUNCTUATOR,
    TOKEN_COMMENT_LINE,
    TOKEN_COMMENT_BLOCK,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    int start_line;
    int start_col;
    int end_line;
    int end_col;
    char *lexeme;
    int error;
    const char *error_kind;
} Token;

typedef struct {
    const char *input;
    size_t length;
    size_t pos;
    int line;
    int col;
} Lexer;

void lexer_init(Lexer *lx, const char *input, size_t length);
Token lexer_next(Lexer *lx);
void token_free(Token *tok);

#endif
