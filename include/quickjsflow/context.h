/* MIT License - Copyright (c) 2026 QuickJSFlow contributors */
#ifndef QUICKJSFLOW_CONTEXT_H
#define QUICKJSFLOW_CONTEXT_H

#include <stddef.h>
#include "quickjsflow/arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * QuickJSFlow Context — the central lifecycle manager.
 *
 * Every parse, transform, or codegen operation requires a context.
 * Contexts are independent; multiple contexts can be used concurrently
 * from different threads without external synchronization.
 *
 * All AST nodes created within a context are allocated from the
 * context's arena. Destroying the context frees all associated memory.
 *
 * Lifecycle:
 *   qjsf_context_t *ctx = qjsf_context_new();
 *   // ... parse, transform, codegen ...
 *   qjsf_context_free(ctx);
 */

/* Forward declaration (opaque handle) */
typedef struct qjsf_context_s qjsf_context_t;

/**
 * Error codes returned by library functions.
 */
typedef enum {
    QJSF_OK = 0,
    QJSF_ERR_INVALID_SYNTAX,
    QJSF_ERR_UNEXPECTED_EOF,
    QJSF_ERR_INVALID_TOKEN,
    QJSF_ERR_OUT_OF_MEMORY,
    QJSF_ERR_NULL_CONTEXT,
    QJSF_ERR_NULL_INPUT,
    QJSF_ERR_CODEGEN_FAILED,
    QJSF_ERR_INTERNAL,
} qjsf_error_t;

/**
 * Detailed error information.
 */
typedef struct {
    qjsf_error_t code;
    char message[256];
    int line;
    int column;
} qjsf_error_info_t;

/**
 * Configuration options passed at context creation time.
 */
typedef struct {
    /** Initial arena block size (0 = default 64KB) */
    size_t arena_block_size;

    /** Maximum AST depth before error (0 = default 1024) */
    size_t max_ast_depth;

    /** Maximum string literal length (0 = default 1MB) */
    size_t max_string_length;

    /** Enable source map generation during codegen */
    int enable_source_map;
} qjsf_context_config_t;

/**
 * Create a new context with default configuration.
 * Returns NULL on allocation failure.
 */
qjsf_context_t *qjsf_context_new(void);

/**
 * Create a new context with custom configuration.
 * `config` may be NULL for defaults.
 * Returns NULL on allocation failure.
 */
qjsf_context_t *qjsf_context_new_with_config(const qjsf_context_config_t *config);

/**
 * Free a context and all associated memory (arena, AST nodes, error info).
 */
void qjsf_context_free(qjsf_context_t *ctx);

/**
 * Get the arena associated with this context.
 * Useful for custom allocation within the context's lifetime.
 */
Arena *qjsf_context_get_arena(qjsf_context_t *ctx);

/**
 * Get the last error information.
 * Returns NULL if no error has occurred.
 */
const qjsf_error_info_t *qjsf_context_get_error(const qjsf_context_t *ctx);

/**
 * Set error information on a context.
 * Used internally by library functions.
 */
void qjsf_context_set_error(qjsf_context_t *ctx, qjsf_error_t code,
                            const char *message, int line, int column);

/**
 * Clear any stored error.
 */
void qjsf_context_clear_error(qjsf_context_t *ctx);

/**
 * Get library version string (e.g., "1.0.0").
 */
const char *qjsf_version_string(void);

/**
 * Get library version components.
 */
void qjsf_version_info(int *major, int *minor, int *patch);

#define QJSF_VERSION_MAJOR 0
#define QJSF_VERSION_MINOR 1
#define QJSF_VERSION_PATCH 0
#define QJSF_ABI_VERSION   0

#ifdef __cplusplus
}
#endif

#endif /* QUICKJSFLOW_CONTEXT_H */
