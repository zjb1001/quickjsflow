#ifndef TEST_FRAMEWORK_EXTENDED_H
#define TEST_FRAMEWORK_EXTENDED_H

/**
 * @file test_framework_extended.h
 * @brief Extended testing framework with snapshots, round-trip, and comparison utilities
 * 
 * Provides:
 * - Enhanced assertions (struct comparison, near-matching)
 * - Snapshot testing (capture expected output)
 * - Round-trip verification (parse → generate → parse → compare)
 * - JSON/diff output for debugging
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Enhanced Test Statistics
 * ============================================================================ */

typedef enum {
    TEST_PASSED = 0,
    TEST_FAILED = 1,
    TEST_SKIPPED = 2
} TestStatus;

typedef struct {
    int passed;
    int failed;
    int skipped;
    int assertions;
    struct {
        const char *name;
        TestStatus status;
    } *results;  // dynamic array
    int results_count;
    int results_capacity;
} ExtendedTestStats;

extern ExtendedTestStats test_stats;

/* ============================================================================
 * Assertion Macros (Enhanced)
 * ============================================================================ */

#define ASSERT_EQ(a, b, msg) do { \
    test_stats.assertions++; \
    if ((a) != (b)) { \
        fprintf(stderr, "FAIL: %s (expected %d, got %d) at %s:%d\n", \
            msg, (int)(b), (int)(a), __FILE__, __LINE__); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define ASSERT_NE(a, b, msg) do { \
    test_stats.assertions++; \
    if ((a) == (b)) { \
        fprintf(stderr, "FAIL: %s (expected != %d, got %d) at %s:%d\n", \
            msg, (int)(a), (int)(b), __FILE__, __LINE__); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define ASSERT_STR_EQ(a, b, msg) do { \
    test_stats.assertions++; \
    const char *_a = (a) ? (a) : ""; \
    const char *_b = (b) ? (b) : ""; \
    if (strcmp(_a, _b) != 0) { \
        fprintf(stderr, "FAIL: %s\n  expected: '%s'\n  got:      '%s'\n  at %s:%d\n", \
            msg, _b, _a, __FILE__, __LINE__); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define ASSERT_STRN_EQ(a, b, n, msg) do { \
    test_stats.assertions++; \
    if (strncmp((a) ? (a) : "", (b) ? (b) : "", (n)) != 0) { \
        fprintf(stderr, "FAIL: %s (first %zu bytes mismatch) at %s:%d\n", \
            msg, (n), __FILE__, __LINE__); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define ASSERT_NOT_NULL(a, msg) do { \
    test_stats.assertions++; \
    if (!(a)) { \
        fprintf(stderr, "FAIL: %s (expected non-null) at %s:%d\n", \
            msg, __FILE__, __LINE__); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define ASSERT_NULL(a, msg) do { \
    test_stats.assertions++; \
    if ((a)) { \
        fprintf(stderr, "FAIL: %s (expected null) at %s:%d\n", \
            msg, __FILE__, __LINE__); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define ASSERT_TRUE(a, msg) do { \
    test_stats.assertions++; \
    if (!(a)) { \
        fprintf(stderr, "FAIL: %s (expected true) at %s:%d\n", \
            msg, __FILE__, __LINE__); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

#define ASSERT_FALSE(a, msg) do { \
    test_stats.assertions++; \
    if ((a)) { \
        fprintf(stderr, "FAIL: %s (expected false) at %s:%d\n", \
            msg, __FILE__, __LINE__); \
        test_stats.failed++; \
    } else { \
        test_stats.passed++; \
    } \
} while (0)

/* ============================================================================
 * Test Grouping and Lifecycle
 * ============================================================================ */

typedef struct {
    const char *name;
    void (*setup)(void);
    void (*teardown)(void);
    void (*test)(void);
} TestCase;

/**
 * Register and run a test case with setup/teardown
 */
void run_test_case(const char *name, 
                   void (*setup)(void),
                   void (*teardown)(void),
                   void (*test)(void));

/**
 * Skip a test with a reason
 */
void skip_test(const char *reason);

/* ============================================================================
 * Snapshot Testing
 * ============================================================================ */

/**
 * Compare output against baseline snapshot
 * @param test_name: Name of test (used for snapshot filename)
 * @param actual: Actual output
 * @param expected: Expected output (or NULL to accept/update)
 * @return: 1 if match, 0 if mismatch
 * 
 * Snapshot files stored in: test/snapshots/{test_name}.snapshot
 * On failure, creates: test/snapshots/{test_name}.actual
 */
int snapshot_match(const char *test_name, const char *actual);

/**
 * Update snapshot with new expected value
 */
void snapshot_update(const char *test_name, const char *actual);

/**
 * Load expected snapshot
 * @param test_name: Test name
 * @return: Snapshot content, or NULL if not found (must be freed)
 */
char *snapshot_load(const char *test_name);

/**
 * Save snapshot to disk
 */
void snapshot_save(const char *test_name, const char *content);

/* ============================================================================
 * Round-Trip Testing
 * ============================================================================ */

/**
 * Round-trip test specification
 */
typedef struct {
    const char *name;
    const char *source_code;
    int should_parse;        // 1 if parse should succeed
    int preserve_formatting; // 1 if formatting must be preserved
} RoundTripTest;

/**
 * Execute round-trip: parse → generate → parse → verify structure matches
 * 
 * @param source: Original JavaScript source
 * @param test_name: Name for error reporting
 * @return: 1 if round-trip successful, 0 otherwise
 * 
 * Procedure:
 * 1. Parse original source → AST1
 * 2. Generate code from AST1 → code2
 * 3. Parse code2 → AST2
 * 4. Compare AST1 and AST2 structurally
 * 5. Report mismatches
 */
int roundtrip_test(const char *source, const char *test_name);

/**
 * Execute round-trip with more detailed reporting
 * @param source: Original code
 * @param test_name: Test name
 * @param report: Output parameter for detailed report (allocate before call)
 * @return: 1 if success, 0 if failure
 */
typedef struct {
    int success;
    char *original_code;
    char *generated_code;
    int node_count_1;
    int node_count_2;
    const char *mismatch_reason;
} RoundTripReport;

int roundtrip_test_detailed(const char *source, const char *test_name,
                           RoundTripReport *report);

void roundtrip_report_free(RoundTripReport *report);

/* ============================================================================
 * AST Comparison
 * ============================================================================ */

/**
 * Deep comparison of two AST nodes
 * @param node1: First AST node
 * @param node2: Second AST node
 * @return: 1 if structurally identical, 0 otherwise
 */
int ast_nodes_equal(const AstNode *node1, const AstNode *node2);

/**
 * Compare AST trees and collect differences
 * @param node1: First tree
 * @param node2: Second tree
 * @param differences: Output array of difference strings (must allocate)
 * @param max_differences: Size of difference array
 * @return: Number of differences found
 */
int ast_compare_detailed(const AstNode *node1, const AstNode *node2,
                        char **differences, int max_differences);

/**
 * Serialize AST to JSON for comparison
 * @param node: AST node to serialize
 * @return: JSON string (caller must free)
 */
char *ast_to_json(const AstNode *node);

/**
 * Pretty-print AST for debugging
 * @param node: AST node
 * @param indent: Initial indentation
 * @return: Allocated string (must be freed)
 */
char *ast_to_string(const AstNode *node, int indent);

/* ============================================================================
 * Test Summary and Output
 * ============================================================================ */

#define TEST_SUMMARY() do { \
    printf("\n========================================\n"); \
    printf("Test Results:\n"); \
    printf("  Passed:   %d\n", test_stats.passed); \
    printf("  Failed:   %d\n", test_stats.failed); \
    printf("  Skipped:  %d\n", test_stats.skipped); \
    printf("  Total:    %d\n", test_stats.assertions); \
    printf("========================================\n"); \
    return test_stats.failed > 0 ? 1 : 0; \
} while (0)

/**
 * Print detailed test report
 */
void test_summary_detailed(FILE *fp);

/**
 * Export test results as JSON string
 * @return: JSON string (caller must free)
 */
char *test_results_to_json(void);

/**
 * Initialize test framework
 */
void test_framework_init(void);

/**
 * Cleanup test framework
 */
void test_framework_cleanup(void);

#endif // TEST_FRAMEWORK_EXTENDED_H
