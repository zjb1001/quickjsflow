/* MIT License - Copyright (c) 2026 QuickJSFlow contributors */
#ifndef QUICKJSFLOW_DIFF_H
#define QUICKJSFLOW_DIFF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Diff operation types.
 */
typedef enum {
    DIFF_KEEP = 0,   // unchanged line
    DIFF_INSERT,     // line added in new text
    DIFF_DELETE,     // line removed from old text
} DiffOpType;

/**
 * A single diff operation (one line).
 */
typedef struct {
    DiffOpType op;
    size_t old_line;   // 0-based line number in old text (DIFF_DELETE/DIFF_KEEP)
    size_t new_line;   // 0-based line number in new text (DIFF_INSERT/DIFF_KEEP)
    const char *text;  // pointer into original text (not owned)
} DiffLine;

/**
 * A diff result — array of DiffLine operations.
 */
typedef struct {
    DiffLine *lines;
    size_t count;
    size_t capacity;
} DiffResult;

/**
 * Compute line-based diff between two texts using Myers' algorithm.
 *
 * @param old_text Old source text
 * @param old_len  Length of old text
 * @param new_text New source text
 * @param new_len  Length of new text
 * @return DiffResult with array of DiffLine operations.
 *         Caller must free with diff_result_free().
 */
DiffResult diff_compute(const char *old_text, size_t old_len,
                        const char *new_text, size_t new_len);

/**
 * Free a diff result.
 */
void diff_result_free(DiffResult *result);

/**
 * Count the number of changed lines (inserts + deletes) in a diff.
 */
size_t diff_change_count(const DiffResult *result);

/**
 * Check if two texts are identical (no changes).
 */
int diff_is_unchanged(const DiffResult *result);

/**
 * Get a human-readable summary of the diff.
 * Returns a heap-allocated string. Caller must free().
 */
char *diff_summary(const DiffResult *result);

#ifdef __cplusplus
}
#endif

#endif /* QUICKJSFLOW_DIFF_H */
