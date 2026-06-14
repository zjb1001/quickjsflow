/* MIT License - Copyright (c) 2026 QuickJSFlow contributors */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "quickjsflow/context.h"

struct qjsf_context_s {
    Arena *arena;
    qjsf_error_info_t last_error;
    int has_error;
    qjsf_context_config_t config;
};

/* Default config */
static const qjsf_context_config_t default_config = {
    .arena_block_size  = 64 * 1024,      /* 64KB */
    .max_ast_depth     = 1024,
    .max_string_length = 1024 * 1024,     /* 1MB */
    .enable_source_map = 0,
};

qjsf_context_t *qjsf_context_new(void) {
    return qjsf_context_new_with_config(NULL);
}

qjsf_context_t *qjsf_context_new_with_config(const qjsf_context_config_t *config) {
    qjsf_context_t *ctx = (qjsf_context_t *)malloc(sizeof(qjsf_context_t));
    if (!ctx) return NULL;

    /* Copy config or use defaults */
    if (config) {
        ctx->config = *config;
        /* Ensure reasonable defaults for zero values */
        if (ctx->config.arena_block_size == 0)
            ctx->config.arena_block_size = default_config.arena_block_size;
        if (ctx->config.max_ast_depth == 0)
            ctx->config.max_ast_depth = default_config.max_ast_depth;
        if (ctx->config.max_string_length == 0)
            ctx->config.max_string_length = default_config.max_string_length;
    } else {
        ctx->config = default_config;
    }

    /* Create the arena */
    ctx->arena = arena_create_with_size(ctx->config.arena_block_size);
    if (!ctx->arena) {
        free(ctx);
        return NULL;
    }

    ctx->has_error = 0;
    memset(&ctx->last_error, 0, sizeof(ctx->last_error));

    return ctx;
}

void qjsf_context_free(qjsf_context_t *ctx) {
    if (!ctx) return;
    arena_destroy(ctx->arena);
    free(ctx);
}

Arena *qjsf_context_get_arena(qjsf_context_t *ctx) {
    return ctx ? ctx->arena : NULL;
}

const qjsf_error_info_t *qjsf_context_get_error(const qjsf_context_t *ctx) {
    if (!ctx || !ctx->has_error) return NULL;
    return &ctx->last_error;
}

void qjsf_context_set_error(qjsf_context_t *ctx, qjsf_error_t code,
                            const char *message, int line, int column) {
    if (!ctx) return;
    ctx->has_error = 1;
    ctx->last_error.code = code;
    ctx->last_error.line = line;
    ctx->last_error.column = column;
    if (message) {
        strncpy(ctx->last_error.message, message, sizeof(ctx->last_error.message) - 1);
        ctx->last_error.message[sizeof(ctx->last_error.message) - 1] = '\0';
    } else {
        ctx->last_error.message[0] = '\0';
    }
}

void qjsf_context_clear_error(qjsf_context_t *ctx) {
    if (!ctx) return;
    ctx->has_error = 0;
}

#define QJSF_VERSION_STR "0.1.0"

const char *qjsf_version_string(void) {
    return QJSF_VERSION_STR;
}

void qjsf_version_info(int *major, int *minor, int *patch) {
    if (major) *major = QJSF_VERSION_MAJOR;
    if (minor) *minor = QJSF_VERSION_MINOR;
    if (patch) *patch = QJSF_VERSION_PATCH;
}
