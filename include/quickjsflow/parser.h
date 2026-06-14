#ifndef QUICKJSFLOW_PARSER_H
#define QUICKJSFLOW_PARSER_H

#include <stddef.h>
#include "quickjsflow/ast.h"
#include "quickjsflow/lexer.h"

typedef struct {
    Lexer lx;
    Token lookahead;
    int has_lookahead;
    Program *comment_sink; // populated during parse_program
    Arena *arena;           // optional: Arena for AST node allocation (NULL = heap)
} Parser;

void parser_init(Parser *p, const char *input, size_t length);

/* Set an Arena for AST node allocation. Call BEFORE parse_program().
 * When set, AstNode wrappers are allocated from the arena (O(1) bulk free).
 * Payload structs continue using heap allocation. */
void parser_set_arena(Parser *p, Arena *arena);

AstNode *parse_program(Parser *p);

/* High-level Context-based parse API (Issue 14).
 * All AST nodes are allocated from ctx's arena. */
struct qjsf_context_s;
AstNode *qjsf_parse_string(struct qjsf_context_s *ctx, const char *source, size_t length);

#endif
