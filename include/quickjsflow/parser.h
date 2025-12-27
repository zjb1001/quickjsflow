#ifndef QUICKJSFLOW_PARSER_H
#define QUICKJSFLOW_PARSER_H

#include <stddef.h>
#include "quickjsflow/ast.h"
#include "quickjsflow/lexer.h"

typedef struct {
    Lexer lx;
    Token lookahead;
    int has_lookahead;
    struct Program *comment_sink; // populated during parse_program
} Parser;

void parser_init(Parser *p, const char *input, size_t length);
AstNode *parse_program(Parser *p);

#endif
