#ifndef UTIL_HASHMAP_H
#define UTIL_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include "container.h"
#include "debug.h"
#include "macros.h"

typedef enum {
    HMAP_NO_FLAGS = 0, // For self-documentation purposes only
    HMAP_BORROWED_KEYS = 1 << 0, // Never call free(3) on HashMapEntry::key
} HashMapFlags;

typedef struct {
    char *key;
    void *value;
    size_t hash;
} HashMapEntry;

// A container type for mapping between string keys and pointer values,
// using hashing for primary lookups and quadratic probing for collision
// resolution.
typedef struct {
    HashMapEntry *entries;
    size_t mask; // Length of entries (which is always a power of 2) minus 1
    size_t count; // Number of active entries
    size_t tombstones; // Number of tombstones
    HashMapFlags flags;
} HashMap;

typedef struct {
    const HashMap *map;
    const HashMapEntry *entry;
    size_t idx;
} HashMapIter;

#define HASHMAP_INIT(f) { \
    .entries = NULL, \
    .mask = 0, \
    .count = 0, \
    .tombstones = 0, \
    .flags = f, \
}

static inline HashMapIter hashmap_iter(const HashMap *map)
{
    return (HashMapIter){.map = map};
}

static inline bool hashmap_next(HashMapIter *iter)
{
    const HashMap *map = iter->map;
    if (unlikely(!map->entries)) {
        return false;
    }

    for (size_t i = iter->idx, n = map->mask + 1; i < n; i++) {
        const HashMapEntry *e = map->entries + i;
        if (e->key) {
            iter->entry = e;
            iter->idx = i + 1;
            return true;
        }
    }
    return false;
}

void hashmap_init(HashMap *map, size_t capacity, HashMapFlags flags) NONNULL_ARGS;
void *hashmap_insert(HashMap *map, char *key, void *value) NONNULL_ARGS_AND_RETURN;
void *hashmap_insert_or_replace(HashMap *map, char *key, void *value) NONNULL_ARGS WARN_UNUSED_RESULT;
void *hashmap_remove(HashMap *map, const char *key) NONNULL_ARGS WARN_UNUSED_RESULT;
void hashmap_clear(HashMap *map, FreeFunction free_value) NONNULL_ARG(1);
void hashmap_free(HashMap *map, FreeFunction free_value) NONNULL_ARG(1);
HashMapEntry *hashmap_find(const HashMap *map, const char *key) NONNULL_ARGS WARN_UNUSED_RESULT;

NONNULL_ARGS WARN_UNUSED_RESULT
static inline void *hashmap_get(const HashMap *map, const char *key)
{
    HashMapEntry *entry = hashmap_find(map, key);
    return entry ? entry->value : NULL;
}

NONNULL_ARGS_AND_RETURN WARN_UNUSED_RESULT
static inline void *hashmap_xget(const HashMap *map, const char *key)
{
    void *val = hashmap_get(map, key);
    BUG_ON(!val);
    return val;
}

#endif
