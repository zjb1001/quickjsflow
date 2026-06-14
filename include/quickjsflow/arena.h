/* MIT License - Copyright (c) 2026 QuickJSFlow contributors */
#ifndef QUICKJSFLOW_ARENA_H
#define QUICKJSFLOW_ARENA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Arena (Region-based) Allocator
 *
 * A simple linear allocator for fast, fragmentation-free memory management.
 * All allocations come from a pre-allocated block; individual frees are no-ops.
 * The entire arena is freed at once via `arena_destroy()`.
 *
 * Design goals:
 *   - O(1) allocation (pointer bump)
 *   - O(1) cleanup (single free)
 *   - Predictable memory usage (~1.0-1.5x source code size for AST)
 *   - Cache-friendly (contiguous allocations)
 */

#define ARENA_DEFAULT_BLOCK_SIZE (64 * 1024)      // 64KB default block
#define ARENA_MAX_BLOCK_SIZE     (1024 * 1024 * 16) // 16MB max block

typedef struct ArenaBlock {
    char *memory;
    size_t capacity;
    size_t used;
    struct ArenaBlock *next;
} ArenaBlock;

typedef struct {
    ArenaBlock *head;         // first block
    ArenaBlock *current;      // current block for allocation
    size_t total_allocated;   // total bytes across all blocks
    size_t total_capacity;    // total capacity across all blocks
    size_t block_count;       // number of blocks
    size_t alloc_count;       // number of individual allocations (stats)
} Arena;

/**
 * Create a new arena with the default block size.
 * Returns NULL on allocation failure.
 */
Arena *arena_create(void);

/**
 * Create a new arena with a specified initial block size.
 * `block_size` is clamped to [4096, ARENA_MAX_BLOCK_SIZE].
 * Returns NULL on allocation failure.
 */
Arena *arena_create_with_size(size_t block_size);

/**
 * Allocate `size` bytes from the arena with `alignment`-byte alignment.
 * alignment must be a power of 2 (typically 8 or 16).
 * Returns NULL if the arena cannot grow (out of memory).
 */
void *arena_alloc(Arena *arena, size_t size, size_t alignment);

/**
 * Convenience: allocate with default alignment (sizeof(void*)).
 */
void *arena_alloc_default(Arena *arena, size_t size);

/**
 * Duplicate a string into the arena.
 * Returns NULL on failure.
 */
char *arena_strdup(Arena *arena, const char *str);

/**
 * Duplicate a counted string into the arena.
 * Returns NULL on failure.
 */
char *arena_strndup(Arena *arena, const char *str, size_t n);

/**
 * Reset the arena: mark all blocks as empty without freeing memory.
 * All previously returned pointers become invalid.
 */
void arena_reset(Arena *arena);

/**
 * Destroy the arena and free all memory.
 * All pointers from this arena become invalid.
 */
void arena_destroy(Arena *arena);

/**
 * Get statistics about the arena.
 */
void arena_stats(const Arena *arena,
                 size_t *total_allocated,
                 size_t *total_capacity,
                 size_t *block_count,
                 size_t *alloc_count);

#ifdef __cplusplus
}
#endif

#endif /* QUICKJSFLOW_ARENA_H */
