#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "quickjsflow/arena.h"

static int passed = 0;
static int failed = 0;

#define TEST(name) printf("  [TEST] %s ... ", name)
#define PASS()    do { printf("PASS\n"); passed++; } while (0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); failed++; } while (0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)

static void test_create_destroy(void) {
    TEST("create/destroy");
    Arena *a = arena_create();
    CHECK(a != NULL, "arena_create returned NULL");
    size_t cap = 0, alloc = 0, blocks = 0, count = 0;
    arena_stats(a, &alloc, &cap, &blocks, &count);
    CHECK(cap >= 4096, "default capacity too small");
    CHECK(blocks == 1, "should have 1 block");
    CHECK(alloc == 0, "initial allocated should be 0");
    arena_destroy(a);
    PASS();
}

static void test_basic_alloc(void) {
    TEST("basic alloc");
    Arena *a = arena_create();
    CHECK(a != NULL, "arena_create failed");

    int *p = (int *)arena_alloc_default(a, sizeof(int));
    CHECK(p != NULL, "alloc returned NULL");
    *p = 42;

    double *d = (double *)arena_alloc(a, sizeof(double), 8);
    CHECK(d != NULL, "aligned alloc returned NULL");
    *d = 3.14;

    size_t alloc = 0;
    arena_stats(a, &alloc, NULL, NULL, NULL);
    CHECK(alloc >= sizeof(int) + sizeof(double), "alloc counter wrong");

    /* Verify values survived */
    CHECK(*p == 42, "int value corrupted");
    CHECK(*d == 3.14, "double value corrupted");

    arena_destroy(a);
    PASS();
}

static void test_alignment(void) {
    TEST("alignment");
    Arena *a = arena_create();
    CHECK(a != NULL, "arena_create failed");

    void *p1 = arena_alloc(a, 1, 16);
    CHECK(p1 != NULL, "16-byte aligned alloc failed");
    {
        uintptr_t addr1 = (uintptr_t)p1;
        CHECK((addr1 & 15) == 0, "not 16-byte aligned");
    }

    void *p2 = arena_alloc(a, 1, 8);
    CHECK(p2 != NULL, "8-byte aligned alloc failed");
    {
        uintptr_t addr2 = (uintptr_t)p2;
        CHECK((addr2 & 7) == 0, "not 8-byte aligned");
    }

    arena_destroy(a);
    PASS();
}

static void test_strdup(void) {
    TEST("strdup");
    Arena *a = arena_create();
    CHECK(a != NULL, "arena_create failed");

    const char *orig = "hello, arena!";
    char *copy = arena_strdup(a, orig);
    CHECK(copy != NULL, "strdup returned NULL");
    CHECK(strcmp(copy, orig) == 0, "strdup content mismatch");
    CHECK(copy != orig, "strdup should return new pointer");

    /* strndup */
    char *partial = arena_strndup(a, orig, 5);
    CHECK(partial != NULL, "strndup returned NULL");
    CHECK(strcmp(partial, "hello") == 0, "strndup content mismatch");

    /* NULL input */
    CHECK(arena_strdup(a, NULL) == NULL, "strdup(NULL) should return NULL");

    arena_destroy(a);
    PASS();
}

static void test_reset(void) {
    TEST("reset");
    Arena *a = arena_create();
    CHECK(a != NULL, "arena_create failed");

    char *p1 = (char *)arena_alloc_default(a, 1024);
    CHECK(p1 != NULL, "first alloc failed");
    memset(p1, 'A', 1024);

    arena_reset(a);

    size_t alloc = 0;
    arena_stats(a, &alloc, NULL, NULL, NULL);
    CHECK(alloc == 0, "alloc should be 0 after reset");

    /* Allocate again - should reuse memory */
    char *p2 = (char *)arena_alloc_default(a, 1024);
    CHECK(p2 != NULL, "post-reset alloc failed");
    CHECK(p2 == p1, "post-reset alloc should reuse same memory");

    arena_destroy(a);
    PASS();
}

static void test_multi_block(void) {
    TEST("multi-block");
    /* Create arena with a very small block to force multiple blocks */
    Arena *a = arena_create_with_size(4096);
    CHECK(a != NULL, "arena_create_with_size failed");

    /* Allocate more than one block's worth */
    for (int i = 0; i < 10; i++) {
        void *p = arena_alloc_default(a, 1024);
        CHECK(p != NULL, "alloc in multi-block failed");
    }

    size_t blocks = 0, cap = 0;
    arena_stats(a, NULL, &cap, &blocks, NULL);
    CHECK(blocks > 1, "should have multiple blocks");
    CHECK(cap > 4096, "capacity should have grown");

    arena_destroy(a);
    PASS();
}

static void test_zero_alloc(void) {
    TEST("zero alloc");
    Arena *a = arena_create();
    CHECK(a != NULL, "arena_create failed");

    CHECK(arena_alloc(a, 0, 8) == NULL, "alloc(0) should return NULL");
    CHECK(arena_alloc_default(a, 0) == NULL, "alloc_default(0) should return NULL");

    arena_destroy(a);
    PASS();
}

static void test_null_arena(void) {
    TEST("null arena safety");
    CHECK(arena_alloc(NULL, 64, 8) == NULL, "arena_alloc(NULL) should return NULL");
    CHECK(arena_alloc_default(NULL, 64) == NULL, "arena_alloc_default(NULL) should return NULL");
    CHECK(arena_strdup(NULL, "x") == NULL, "arena_strdup(NULL) should return NULL");
    arena_reset(NULL);  /* should not crash */
    arena_destroy(NULL); /* should not crash */

    size_t x = 99;
    arena_stats(NULL, &x, NULL, NULL, NULL);
    CHECK(x == 0, "arena_stats(NULL) should set to 0");

    PASS();
}

#include "quickjsflow/parser.h"

static void test_arena_parse(void) {
    TEST("arena-backed parse");
    Arena *a = arena_create();
    CHECK(a != NULL, "arena_create failed");

    /* Parse with arena */
    Parser p;
    parser_init(&p, "var x = 42;", 11);
    parser_set_arena(&p, a);
    AstNode *ast = parse_program(&p);
    CHECK(ast != NULL, "arena parse returned NULL");
    CHECK(ast->type == AST_Program, "root should be Program");

    /* Verify the arena was used (alloc count > 0) */
    size_t alloc_count = 0;
    arena_stats(a, NULL, NULL, NULL, &alloc_count);
    CHECK(alloc_count > 0, "arena should have allocations");

    /* Parse WITHOUT arena (heap fallback) — should still work */
    Parser p2;
    parser_init(&p2, "let y = 99;", 10);
    /* Don't set arena — uses heap */
    AstNode *ast2 = parse_program(&p2);
    CHECK(ast2 != NULL, "heap parse returned NULL");
    CHECK(ast2->type == AST_Program, "heap root should be Program");

    /* Cleanup: free heap node normally, arena node via arena destroy */
    ast_free(ast2);
    arena_destroy(a);  /* frees ast1 nodes */
    PASS();
}

int main(void) {
    printf("\n=== Arena Allocator Tests ===\n\n");
    test_create_destroy();
    test_basic_alloc();
    test_alignment();
    test_strdup();
    test_reset();
    test_multi_block();
    test_zero_alloc();
    test_null_arena();
    test_arena_parse();
    printf("\nResults: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
