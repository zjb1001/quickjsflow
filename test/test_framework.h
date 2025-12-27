#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int passed;
    int failed;
} TestStats;

static TestStats test_stats = {0, 0};

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        fprintf(stderr, "FAIL: %s (expected %d, got %d)\n", msg, (int)(b), (int)(a)); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define ASSERT_STR_EQ(a, b, msg) do { \
    if (strcmp((a) ? (a) : "", (b) ? (b) : "") != 0) { \
        fprintf(stderr, "FAIL: %s (expected '%s', got '%s')\n", msg, (b) ? (b) : "(null)", (a) ? (a) : "(null)"); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define ASSERT_NOT_NULL(a, msg) do { \
    if (!(a)) { \
        fprintf(stderr, "FAIL: %s (expected non-null, got NULL)\n", msg); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define TEST_SUMMARY() do { \
    printf("\n========================================\n"); \
    printf("Tests Passed: %d\n", test_stats.passed); \
    printf("Tests Failed: %d\n", test_stats.failed); \
    printf("========================================\n"); \
    return test_stats.failed > 0 ? 1 : 0; \
} while (0)

#endif
