#ifndef UTIL_INTMAP_H
#define UTIL_INTMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "macros.h"

extern const char tombstone[16];

typedef void (*FreeFunction)(void *ptr);

typedef struct {
    uint32_t key;
    void *value;
} IntMapEntry;

// This is much like the HashMap type from hashmap.h, but for uint32_t
// values and with some of the internal details (different hash function,
// different tombstone handling, not storing hash values in entries, etc.)
// tweaked accordingly.
typedef struct {
    IntMapEntry *entries;
    size_t mask; // Length of entries (which is always a power of 2) minus 1
    size_t count; // Number of active entries
    size_t tombstones; // Number of tombstones
} IntMap;

typedef struct {
    const IntMap *map;
    IntMapEntry *entry;
    size_t idx;
} IntMapIter;

#define INTMAP_INIT { \
    .entries = NULL, \
    .mask = 0, \
    .count = 0, \
    .tombstones = 0 \
}

static inline IntMapIter intmap_iter(const IntMap *map)
{
    return (IntMapIter){.map = map};
}

static inline bool intmap_next(IntMapIter *iter)
{
    const IntMap *map = iter->map;
    if (unlikely(!map->entries)) {
        return false;
    }

    for (size_t i = iter->idx, n = map->mask + 1; i < n; i++) {
        IntMapEntry *e = map->entries + i;
        if (e->value && e->value != tombstone) {
            iter->entry = e;
            iter->idx = i + 1;
            return true;
        }
    }
    return false;
}

void intmap_init(IntMap *map, size_t capacity) NONNULL_ARGS;
void *intmap_insert_or_replace(IntMap *map, uint32_t key, void *value) NONNULL_ARGS;
void *intmap_remove(IntMap *map, uint32_t key) NONNULL_ARGS;
void intmap_free(IntMap *map, FreeFunction free_value) NONNULL_ARG(1);
IntMapEntry *intmap_find(const IntMap *map, uint32_t key) NONNULL_ARGS WARN_UNUSED_RESULT;

static inline void *intmap_get(const IntMap *map, uint32_t key)
{
    IntMapEntry *e = intmap_find(map, key);
    return e ? e->value : NULL;
}

#endif
