/* MIT License - Copyright (c) 2026 QuickJSFlow contributors */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "quickjsflow/diff.h"

/* ---- Line splitting helpers ---- */

typedef struct {
    const char **lines;
    size_t count;
    size_t cap;
} LineVec;

static void lv_init(LineVec *lv) {
    lv->lines = NULL;
    lv->count = 0;
    lv->cap = 0;
}

static void lv_push(LineVec *lv, const char *line) {
    if (lv->count + 1 > lv->cap) {
        size_t cap = lv->cap ? lv->cap * 2 : 256;
        const char **p = (const char **)realloc(lv->lines, cap * sizeof(const char *));
        if (!p) return;
        lv->lines = p;
        lv->cap = cap;
    }
    lv->lines[lv->count++] = line;
}

static void lv_free(LineVec *lv) {
    free(lv->lines);
    lv->lines = NULL;
    lv->count = lv->cap = 0;
}

/* Split text into lines. Returns line count, fills lv. */
static size_t split_lines(const char *text, size_t len, LineVec *lv) {
    if (!text || len == 0) return 0;
    const char *start = text;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n') {
            lv_push(lv, start);
            start = text + i + 1;
        }
    }
    /* Last line (may not end with \n) */
    if (start < text + len) {
        lv_push(lv, start);
    }
    return lv->count;
}

/* ---- LCS-based line diff (O(N*M) DP with backtracking) ---- */
DiffResult diff_compute(const char *old_text, size_t old_len,
                        const char *new_text, size_t new_len) {
    DiffResult result = {NULL, 0, 0};
    if (!old_text) old_text = "";
    if (!new_text) new_text = "";
    if (old_len == 0) old_len = strlen(old_text);
    if (new_len == 0) new_len = strlen(new_text);

    LineVec a, b;
    lv_init(&a); lv_init(&b);
    size_t n = split_lines(old_text, old_len, &a);
    size_t m = split_lines(new_text, new_len, &b);

    /* Allocate result buffer */
    result.capacity = (n > m ? n : m) + 64;
    result.lines = (DiffLine *)calloc(result.capacity, sizeof(DiffLine));
    if (!result.lines) { lv_free(&a); lv_free(&b); return result; }

    /* Use simple O(N*M) LCS-based diff — correct and reliable */
    /* Build LCS table on stack for small diffs, heap for larger */
    size_t dp_size = (n + 1) * (m + 1);
    unsigned short *dp = NULL;
    unsigned short dp_stack[4096];
    if (dp_size <= 4096) {
        dp = dp_stack;
        memset(dp, 0, dp_size * sizeof(unsigned short));
    } else {
        dp = (unsigned short *)calloc(dp_size, sizeof(unsigned short));
    }
    if (!dp) { free(result.lines); lv_free(&a); lv_free(&b); return result; }

    /* Fill LCS lengths */
    for (size_t i = 1; i <= n; i++) {
        for (size_t j = 1; j <= m; j++) {
            if (strcmp(a.lines[i-1], b.lines[j-1]) == 0) {
                dp[i * (m + 1) + j] = dp[(i-1) * (m + 1) + (j-1)] + 1;
            } else {
                unsigned short up = dp[(i-1) * (m + 1) + j];
                unsigned short left = dp[i * (m + 1) + (j-1)];
                dp[i * (m + 1) + j] = up > left ? up : left;
            }
        }
    }

    /* Backtrack to produce diff */
    size_t i = n, j = m;
    size_t total = n + m;
    DiffLine *tmp = (DiffLine *)calloc(total, sizeof(DiffLine));
    size_t tmp_count = 0;
    if (tmp) {
        while (i > 0 || j > 0) {
            if (i > 0 && j > 0 && strcmp(a.lines[i-1], b.lines[j-1]) == 0) {
                tmp[tmp_count].op = DIFF_KEEP;
                tmp[tmp_count].old_line = i - 1;
                tmp[tmp_count].new_line = j - 1;
                tmp[tmp_count].text = a.lines[i-1];
                tmp_count++; i--; j--;
            } else if (j > 0 && (i == 0 || dp[i * (m + 1) + (j-1)] >= dp[(i-1) * (m + 1) + j])) {
                tmp[tmp_count].op = DIFF_INSERT;
                tmp[tmp_count].new_line = j - 1;
                tmp[tmp_count].text = b.lines[j-1];
                tmp_count++; j--;
            } else {
                tmp[tmp_count].op = DIFF_DELETE;
                tmp[tmp_count].old_line = i - 1;
                tmp[tmp_count].text = a.lines[i-1];
                tmp_count++; i--;
            }
        }

        /* Reverse into result */
        for (size_t k = 0; k < tmp_count; k++) {
            result.lines[result.count++] = tmp[tmp_count - 1 - k];
        }
        free(tmp);
    }

    if (dp != dp_stack) free(dp);
    lv_free(&a);
    lv_free(&b);
    return result;
}

void diff_result_free(DiffResult *result) {
    if (!result) return;
    free(result->lines);
    result->lines = NULL;
    result->count = result->capacity = 0;
}

size_t diff_change_count(const DiffResult *result) {
    if (!result) return 0;
    size_t count = 0;
    for (size_t i = 0; i < result->count; i++) {
        if (result->lines[i].op != DIFF_KEEP) count++;
    }
    return count;
}

int diff_is_unchanged(const DiffResult *result) {
    return diff_change_count(result) == 0;
}

char *diff_summary(const DiffResult *result) {
    if (!result) {
        char *s = (char *)malloc(32);
        if (s) snprintf(s, 32, "diff: (null)");
        return s;
    }
    size_t inserts = 0, deletes = 0;
    for (size_t i = 0; i < result->count; i++) {
        if (result->lines[i].op == DIFF_INSERT) inserts++;
        else if (result->lines[i].op == DIFF_DELETE) deletes++;
    }
    char *s = (char *)malloc(128);
    if (s) snprintf(s, 128, "diff: %zu lines, +%zu -%zu", result->count, inserts, deletes);
    return s;
}
