#ifndef UTIL_HASHMAP_H
#define UTIL_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include "macros.h"

typedef void (*FreeFunction)(void *ptr);

typedef struct {
    char *key;
    void *value;
    size_t hash;
} HashMapEntry;

typedef struct {
    HashMapEntry *entries;
    size_t mask; // Length of entries (which is always a power of 2) minus 1
    size_t count; // Number of active entries
    size_t tombstones; // Number of tombstones
} HashMap;

typedef struct {
    const HashMap *map;
    HashMapEntry *entry;
    size_t idx;
} HashMapIter;

#define HASHMAP_INIT { \
    .entries = NULL, \
    .mask = 0, \
    .count = 0, \
    .tombstones = 0 \
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

    extern char tombstone[16];
    for (size_t i = iter->idx, n = map->mask + 1; i < n; i++) {
        HashMapEntry *e = map->entries + i;
        if (e->key && e->key != tombstone) {
            iter->entry = e;
            iter->idx = i + 1;
            return true;
        }
    }
    return false;
}

void hashmap_init(HashMap *map, size_t capacity) NONNULL_ARGS;
void hashmap_insert(HashMap *map, char *key, void *value) NONNULL_ARGS;
void *hashmap_insert_or_replace(HashMap *map, char *key, void *value) NONNULL_ARGS;
void *hashmap_remove(HashMap *map, const char *key) NONNULL_ARGS;
void hashmap_clear(HashMap *map, FreeFunction free_value) NONNULL_ARG(1);
void hashmap_free(HashMap *map, FreeFunction free_value) NONNULL_ARG(1);
HashMapEntry *hashmap_find(const HashMap *map, const char *key) NONNULL_ARGS WARN_UNUSED_RESULT;

static inline void *hashmap_get(const HashMap *map, const char *key)
{
    HashMapEntry *e = hashmap_find(map, key);
    return e ? e->value : NULL;
}

#endif
