#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/context.h"

static int passed = 0;
static int failed = 0;

#define TEST(name) printf("  [TEST] %s ... ", name)
#define PASS()    do { printf("PASS\n"); passed++; } while (0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); failed++; } while (0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)

static void test_create_free(void) {
    TEST("create/free");
    qjsf_context_t *ctx = qjsf_context_new();
    CHECK(ctx != NULL, "context_new returned NULL");
    qjsf_context_free(ctx);
    PASS();
}

static void test_default_config(void) {
    TEST("default config");
    qjsf_context_t *ctx = qjsf_context_new();
    CHECK(ctx != NULL, "context_new failed");

    Arena *arena = qjsf_context_get_arena(ctx);
    CHECK(arena != NULL, "get_arena returned NULL");

    /* Should have no error initially */
    CHECK(qjsf_context_get_error(ctx) == NULL, "initial error should be NULL");

    qjsf_context_free(ctx);
    PASS();
}

static void test_custom_config(void) {
    TEST("custom config");
    qjsf_context_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.arena_block_size = 8192;
    cfg.max_ast_depth = 512;
    cfg.enable_source_map = 1;

    qjsf_context_t *ctx = qjsf_context_new_with_config(&cfg);
    CHECK(ctx != NULL, "context_new_with_config failed");

    Arena *arena = qjsf_context_get_arena(ctx);
    CHECK(arena != NULL, "get_arena returned NULL");

    size_t cap = 0;
    arena_stats(arena, NULL, &cap, NULL, NULL);
    CHECK(cap == 8192, "custom block size not applied");

    qjsf_context_free(ctx);
    PASS();
}

static void test_null_config(void) {
    TEST("NULL config (defaults)");
    qjsf_context_t *ctx = qjsf_context_new_with_config(NULL);
    CHECK(ctx != NULL, "context_new_with_config(NULL) failed");

    Arena *arena = qjsf_context_get_arena(ctx);
    CHECK(arena != NULL, "get_arena returned NULL");
    CHECK(qjsf_context_get_error(ctx) == NULL, "error should be NULL");

    qjsf_context_free(ctx);
    PASS();
}

static void test_error_handling(void) {
    TEST("error handling");
    qjsf_context_t *ctx = qjsf_context_new();
    CHECK(ctx != NULL, "context_new failed");

    /* Set an error */
    qjsf_context_set_error(ctx, QJSF_ERR_INVALID_SYNTAX, "Unexpected token }", 10, 5);

    const qjsf_error_info_t *err = qjsf_context_get_error(ctx);
    CHECK(err != NULL, "get_error should return non-NULL after set");
    CHECK(err->code == QJSF_ERR_INVALID_SYNTAX, "error code mismatch");
    CHECK(err->line == 10, "error line mismatch");
    CHECK(err->column == 5, "error column mismatch");

    /* Clear */
    qjsf_context_clear_error(ctx);
    CHECK(qjsf_context_get_error(ctx) == NULL, "get_error should return NULL after clear");

    qjsf_context_free(ctx);
    PASS();
}

static void test_error_types(void) {
    TEST("error types");
    qjsf_context_t *ctx = qjsf_context_new();
    CHECK(ctx != NULL, "context_new failed");

    /* Verify all error codes are distinct */
    CHECK(QJSF_OK == 0, "QJSF_OK should be 0");
    CHECK(QJSF_ERR_INVALID_SYNTAX != QJSF_ERR_OUT_OF_MEMORY, "error codes should differ");
    CHECK(QJSF_ERR_NULL_CONTEXT != QJSF_ERR_NULL_INPUT, "error codes should differ");

    /* Test error message truncation (long message) */
    char long_msg[512];
    memset(long_msg, 'X', sizeof(long_msg));
    long_msg[511] = '\0';
    qjsf_context_set_error(ctx, QJSF_ERR_INTERNAL, long_msg, 0, 0);

    const qjsf_error_info_t *err = qjsf_context_get_error(ctx);
    CHECK(err != NULL, "error should be present");
    CHECK(strlen(err->message) == 255, "message should be truncated to 255 chars");

    /* Test NULL message */
    qjsf_context_set_error(ctx, QJSF_ERR_OUT_OF_MEMORY, NULL, 0, 0);
    err = qjsf_context_get_error(ctx);
    CHECK(err != NULL && err->code == QJSF_ERR_OUT_OF_MEMORY, "NULL message should work");

    qjsf_context_free(ctx);
    PASS();
}

static void test_null_context_safety(void) {
    TEST("NULL context safety");
    qjsf_context_free(NULL);  /* should not crash */
    CHECK(qjsf_context_get_arena(NULL) == NULL, "get_arena(NULL) should return NULL");
    CHECK(qjsf_context_get_error(NULL) == NULL, "get_error(NULL) should return NULL");
    qjsf_context_set_error(NULL, QJSF_OK, "", 0, 0); /* should not crash */
    qjsf_context_clear_error(NULL); /* should not crash */
    PASS();
}

static void test_version(void) {
    TEST("version API");
    const char *ver = qjsf_version_string();
    CHECK(ver != NULL, "version_string returned NULL");
    CHECK(strlen(ver) > 0, "version string empty");

    int major = 0, minor = 0, patch = 0;
    qjsf_version_info(&major, &minor, &patch);
    CHECK(major == QJSF_VERSION_MAJOR, "major version mismatch");
    CHECK(minor == QJSF_VERSION_MINOR, "minor version mismatch");
    CHECK(patch == QJSF_VERSION_PATCH, "patch version mismatch");

    /* NULL parameters should not crash */
    qjsf_version_info(NULL, NULL, NULL);

    PASS();
}

static void test_multiple_contexts(void) {
    TEST("multiple independent contexts");
    qjsf_context_t *ctx1 = qjsf_context_new();
    qjsf_context_t *ctx2 = qjsf_context_new();
    CHECK(ctx1 != NULL && ctx2 != NULL, "context creation failed");
    CHECK(ctx1 != ctx2, "contexts should be different");

    /* Set error on ctx1 only */
    qjsf_context_set_error(ctx1, QJSF_ERR_INVALID_TOKEN, "bad", 1, 1);
    CHECK(qjsf_context_get_error(ctx1) != NULL, "ctx1 should have error");
    CHECK(qjsf_context_get_error(ctx2) == NULL, "ctx2 should not have error");

    qjsf_context_free(ctx1);
    qjsf_context_free(ctx2);
    PASS();
}

int main(void) {
    printf("\n=== Context Tests ===\n\n");
    test_create_free();
    test_default_config();
    test_custom_config();
    test_null_config();
    test_error_handling();
    test_error_types();
    test_null_context_safety();
    test_version();
    test_multiple_contexts();
    printf("\nResults: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
