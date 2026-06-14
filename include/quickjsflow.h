/* MIT License - Copyright (c) 2026 QuickJSFlow contributors */
#ifndef QUICKJSFLOW_H
#define QUICKJSFLOW_H

/**
 * QuickJSFlow — High-Performance JavaScript AST Library
 * ======================================================
 *
 * A lightweight, embeddable, zero-dependency C library for JavaScript
 * parsing, analysis, transformation, and code generation.
 *
 * Design principles:
 *   - Context-aware & reentrant (thread-safe)
 *   - Arena-based memory management (O(1) alloc/free)
 *   - Transparent AST nodes (cJSON-style direct member access)
 *   - Zero external dependencies (libc only)
 *   - C99 compatible (graceful C11 feature degradation)
 *
 * Quick start:
 *   #include "quickjsflow.h"
 *
 *   qjsf_context_t *ctx = qjsf_context_new();
 *   qjsf_node_t *ast = qjsf_parse_string(ctx, "var x = 42;", 12);
 *   char *code = qjsf_codegen(ctx, ast);
 *   printf("%s\n", code);
 *   qjsf_context_free(ctx);
 *
 * Version: 0.1.0 (Alpha)
 * License: MIT
 */

#include "quickjsflow/context.h"
#include "quickjsflow/arena.h"

/* Forward declarations for types defined in other headers */
/* (included here for single-header convenience) */

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * AST Node Types
 * ================================================================ */

#include "quickjsflow/ast.h"

/* ================================================================
 * Lexer
 * ================================================================ */

#include "quickjsflow/lexer.h"

/* ================================================================
 * Parser
 * ================================================================ */

#include "quickjsflow/parser.h"

/* ================================================================
 * Scope Analysis
 * ================================================================ */

#include "quickjsflow/scope.h"

/* ================================================================
 * AST Editing (Immutable)
 * ================================================================ */

#include "quickjsflow/edit.h"

/* ================================================================
 * Code Generation
 * ================================================================ */

#include "quickjsflow/codegen.h"

/* ================================================================
 * Control Flow Graph
 * ================================================================ */

#include "quickjsflow/cfg.h"

/* ================================================================
 * Plugin System
 * ================================================================ */

#include "quickjsflow/plugin.h"

/* ================================================================
 * High-Level Convenience API
 * ================================================================ */

/**
 * Parse JavaScript source code into an AST.
 * All AST nodes are allocated from the context's arena.
 *
 * @param ctx   Context (must be non-NULL)
 * @param source JavaScript source code (must be non-NULL)
 * @param length Length of source in bytes (0 to auto-detect via strlen)
 * @return Root AST node (Program), or NULL on error.
 *         Check qjsf_context_get_error(ctx) for details.
 */
AstNode *qjsf_parse_string(qjsf_context_t *ctx, const char *source, size_t length);

/**
 * Generate JavaScript source code from an AST.
 * The output code buffer is allocated from the context's arena.
 *
 * @param ctx Context (must be non-NULL)
 * @param root Root AST node (Program)
 * @param options Codegen options, or NULL for defaults
 * @return CodegenResult with .code as arena-allocated string, or .code=NULL on error.
 */
CodegenResult qjsf_codegen(qjsf_context_t *ctx, const AstNode *root, const CodegenOptions *options);

#ifdef __cplusplus
}
#endif

#endif /* QUICKJSFLOW_H */
