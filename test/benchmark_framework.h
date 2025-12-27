#ifndef BENCHMARK_FRAMEWORK_H
#define BENCHMARK_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

// Benchmark timing utilities
typedef struct {
    struct timeval start;
    struct timeval end;
    double elapsed_ms;
} BenchmarkTimer;

static void benchmark_start(BenchmarkTimer* timer) {
    gettimeofday(&timer->start, NULL);
}

static void benchmark_end(BenchmarkTimer* timer) {
    gettimeofday(&timer->end, NULL);
    timer->elapsed_ms = (timer->end.tv_sec - timer->start.tv_sec) * 1000.0 +
                        (timer->end.tv_usec - timer->start.tv_usec) / 1000.0;
}

// Benchmark statistics
typedef struct {
    const char* name;
    double min_ms;
    double max_ms;
    double avg_ms;
    double total_ms;
    size_t iterations;
    size_t bytes_processed;
} BenchmarkResult;

typedef struct {
    BenchmarkResult* results;
    size_t count;
    size_t capacity;
} BenchmarkSuite;

static BenchmarkSuite* benchmark_suite_create(void) {
    BenchmarkSuite* suite = malloc(sizeof(BenchmarkSuite));
    suite->capacity = 16;
    suite->count = 0;
    suite->results = malloc(sizeof(BenchmarkResult) * suite->capacity);
    return suite;
}

static void benchmark_suite_add(BenchmarkSuite* suite, const char* name,
                               double elapsed_ms, size_t bytes) {
    if (suite->count >= suite->capacity) {
        suite->capacity *= 2;
        suite->results = realloc(suite->results, sizeof(BenchmarkResult) * suite->capacity);
    }
    
    BenchmarkResult* result = &suite->results[suite->count];
    result->name = name;
    result->min_ms = elapsed_ms;
    result->max_ms = elapsed_ms;
    result->avg_ms = elapsed_ms;
    result->total_ms = elapsed_ms;
    result->iterations = 1;
    result->bytes_processed = bytes;
    
    suite->count++;
}

static void benchmark_suite_update(BenchmarkSuite* suite, const char* name,
                                  double elapsed_ms, size_t bytes) {
    for (size_t i = 0; i < suite->count; i++) {
        if (strcmp(suite->results[i].name, name) == 0) {
            BenchmarkResult* result = &suite->results[i];
            result->iterations++;
            result->total_ms += elapsed_ms;
            result->avg_ms = result->total_ms / result->iterations;
            if (elapsed_ms < result->min_ms) result->min_ms = elapsed_ms;
            if (elapsed_ms > result->max_ms) result->max_ms = elapsed_ms;
            result->bytes_processed += bytes;
            return;
        }
    }
    benchmark_suite_add(suite, name, elapsed_ms, bytes);
}

static void benchmark_suite_print(BenchmarkSuite* suite, FILE* out) {
    fprintf(out, "\n========== BENCHMARK RESULTS ==========\n\n");
    fprintf(out, "%-30s %10s %10s %10s %10s %12s\n",
            "Benchmark", "Avg (ms)", "Min (ms)", "Max (ms)", "Iters", "MB/s");
    fprintf(out, "----------------------------------------------------------------"
                 "----------\n");
    
    for (size_t i = 0; i < suite->count; i++) {
        BenchmarkResult* r = &suite->results[i];
        double mb_per_sec = 0.0;
        if (r->avg_ms > 0 && r->bytes_processed > 0) {
            mb_per_sec = (r->bytes_processed / (1024.0 * 1024.0)) / (r->avg_ms / 1000.0);
        }
        
        fprintf(out, "%-30s %10.3f %10.3f %10.3f %10zu %12.2f\n",
                r->name, r->avg_ms, r->min_ms, r->max_ms, r->iterations, mb_per_sec);
    }
    
    fprintf(out, "\n========================================\n");
}

static void benchmark_suite_save(BenchmarkSuite* suite, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Failed to open %s for writing\n", filename);
        return;
    }
    benchmark_suite_print(suite, f);
    fclose(f);
}

static void benchmark_suite_free(BenchmarkSuite* suite) {
    free(suite->results);
    free(suite);
}

// Benchmark runner macro
#define BENCHMARK(suite, name, code, bytes) do { \
    BenchmarkTimer timer; \
    benchmark_start(&timer); \
    code; \
    benchmark_end(&timer); \
    benchmark_suite_update(suite, name, timer.elapsed_ms, bytes); \
} while(0)

#define BENCHMARK_N(suite, name, code, bytes, iterations) do { \
    for (size_t _i = 0; _i < iterations; _i++) { \
        BENCHMARK(suite, name, code, bytes); \
    } \
} while(0)

#endif
