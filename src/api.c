/* MIT License - Copyright (c) 2026 QuickJSFlow contributors */
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/context.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/codegen.h"
#include "quickjsflow/arena.h"

/* ================================================================
 * High-Level Context-based API (Issue 14 — Library API Design)
 * ================================================================ */

AstNode *qjsf_parse_string(qjsf_context_t *ctx, const char *source, size_t length) {
    if (!ctx || !source) {
        if (ctx) qjsf_context_set_error(ctx, QJSF_ERR_NULL_INPUT, "source is NULL", 0, 0);
        return NULL;
    }
    if (length == 0) length = strlen(source);
    Arena *arena = qjsf_context_get_arena(ctx);
    if (!arena) {
        qjsf_context_set_error(ctx, QJSF_ERR_NULL_CONTEXT, "context has no arena", 0, 0);
        return NULL;
    }
    Parser p;
    parser_init(&p, source, length);
    parser_set_arena(&p, arena);
    AstNode *ast = parse_program(&p);
    if (!ast) {
        qjsf_context_set_error(ctx, QJSF_ERR_INVALID_SYNTAX, "parse failed", 0, 0);
    }
    return ast;
}

CodegenResult qjsf_codegen(qjsf_context_t *ctx, const AstNode *root, const CodegenOptions *options) {
    CodegenResult result = {NULL, NULL, NULL};
    if (!ctx || !root) {
        if (ctx) qjsf_context_set_error(ctx, QJSF_ERR_NULL_INPUT, "root is NULL", 0, 0);
        return result;
    }
    Arena *arena = qjsf_context_get_arena(ctx);
    if (!arena) {
        qjsf_context_set_error(ctx, QJSF_ERR_NULL_CONTEXT, "context has no arena", 0, 0);
        return result;
    }
    result = codegen_generate(root, options);
    if (!result.code) {
        qjsf_context_set_error(ctx, QJSF_ERR_CODEGEN_FAILED, "codegen failed", 0, 0);
        return result;
    }
    /* Copy output to arena and mark as arena-owned */
    char *arena_code = arena_strdup(arena, result.code);
    if (!arena_code) {
        qjsf_context_set_error(ctx, QJSF_ERR_OUT_OF_MEMORY, "arena alloc failed", 0, 0);
        free(result.code);
        free(result.source_map);
        result.code = NULL;
        result.source_map = NULL;
        return result;
    }
    free(result.code);
    result.code = arena_code;
    result.owner_arena = arena;
    return result;
}
