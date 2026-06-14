/* MIT License - Copyright (c) 2026 QuickJSFlow contributors */
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/arena.h"

/* Round `size` up to the nearest multiple of `alignment` */
static size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

/* Check if `alignment` is a power of 2 */
static int is_power_of_two(size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

static size_t clamp_block_size(size_t size) {
    if (size < 4096) return 4096;
    if (size > ARENA_MAX_BLOCK_SIZE) return ARENA_MAX_BLOCK_SIZE;
    return size;
}

static ArenaBlock *block_create(size_t capacity) {
    ArenaBlock *block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
    if (!block) return NULL;
    block->memory = (char *)malloc(capacity);
    if (!block->memory) {
        free(block);
        return NULL;
    }
    block->capacity = capacity;
    block->used = 0;
    block->next = NULL;
    return block;
}

static void block_destroy(ArenaBlock *block) {
    if (!block) return;
    block_destroy(block->next);
    free(block->memory);
    free(block);
}

Arena *arena_create(void) {
    return arena_create_with_size(ARENA_DEFAULT_BLOCK_SIZE);
}

Arena *arena_create_with_size(size_t block_size) {
    Arena *arena = (Arena *)malloc(sizeof(Arena));
    if (!arena) return NULL;

    block_size = clamp_block_size(block_size);
    ArenaBlock *first = block_create(block_size);
    if (!first) {
        free(arena);
        return NULL;
    }

    arena->head = first;
    arena->current = first;
    arena->total_allocated = 0;
    arena->total_capacity = block_size;
    arena->block_count = 1;
    arena->alloc_count = 0;
    return arena;
}

void *arena_alloc(Arena *arena, size_t size, size_t alignment) {
    if (!arena || size == 0 || !is_power_of_two(alignment)) return NULL;
    /* Reject oversized allocations to prevent integer overflow in needed=size+alignment */
    if (size > ARENA_MAX_BLOCK_SIZE) return NULL;

    size_t offset = align_up(arena->current->used, alignment);

    /* If the current block can't satisfy this allocation, create a new one */
    if (offset + size > arena->current->capacity) {
        /* Determine new block size: max(requested, current * 2, min of max) */
        size_t needed = size + alignment; /* worst case alignment overhead */
        size_t new_capacity = arena->current->capacity * 2;
        if (new_capacity < needed) new_capacity = needed;
        if (new_capacity > ARENA_MAX_BLOCK_SIZE) new_capacity = ARENA_MAX_BLOCK_SIZE;
        if (new_capacity < needed) {
            /* Single allocation too large for max block */
            new_capacity = needed;
        }

        ArenaBlock *new_block = block_create(new_capacity);
        if (!new_block) return NULL;

        new_block->next = arena->current->next;
        arena->current->next = new_block;
        arena->current = new_block;
        arena->total_capacity += new_capacity;
        arena->block_count++;

        offset = 0;
    }

    arena->current->used = offset + size;
    arena->total_allocated += size;
    arena->alloc_count++;
    return arena->current->memory + offset;
}

void *arena_alloc_default(Arena *arena, size_t size) {
    return arena_alloc(arena, size, sizeof(void *));
}

char *arena_strdup(Arena *arena, const char *str) {
    if (!str) return NULL;
    return arena_strndup(arena, str, strlen(str));
}

char *arena_strndup(Arena *arena, const char *str, size_t n) {
    if (!str || n == 0) {
        char *empty = (char *)arena_alloc_default(arena, 1);
        if (empty) empty[0] = '\0';
        return empty;
    }
    char *copy = (char *)arena_alloc_default(arena, n + 1);
    if (!copy) return NULL;
    memcpy(copy, str, n);
    copy[n] = '\0';
    return copy;
}

void arena_reset(Arena *arena) {
    if (!arena) return;
    ArenaBlock *b = arena->head;
    while (b) {
        b->used = 0;
        b = b->next;
    }
    arena->current = arena->head;
    arena->total_allocated = 0;
    arena->alloc_count = 0;
}

void arena_destroy(Arena *arena) {
    if (!arena) return;
    block_destroy(arena->head);
    free(arena);
}

void arena_stats(const Arena *arena,
                 size_t *total_allocated,
                 size_t *total_capacity,
                 size_t *block_count,
                 size_t *alloc_count) {
    if (total_allocated) *total_allocated = arena ? arena->total_allocated : 0;
    if (total_capacity)  *total_capacity  = arena ? arena->total_capacity : 0;
    if (block_count)     *block_count     = arena ? arena->block_count : 0;
    if (alloc_count)     *alloc_count     = arena ? arena->alloc_count : 0;
}
