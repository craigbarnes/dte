#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"
#include "debug.h"
#include "hash.h"
#include "str-util.h"

static char tombstone[16] = "TOMBSTONE";

enum {
    MIN_SIZE = 8,
    MAX_SIZE = (SIZE_MAX / 2) + 1
};

static bool resize(HashMap *map, size_t capacity)
{
    static_assert(IS_POWER_OF_2(MIN_SIZE));
    static_assert(IS_POWER_OF_2(MAX_SIZE));
    if (capacity > MAX_SIZE) {
        capacity = MAX_SIZE;
    }

    size_t newsize = MIN_SIZE;
    while (newsize < capacity) {
        newsize *= 2;
    }

    HashMapEntry *oldtab = map->entries;
    HashMapEntry *oldend = map->entries + map->mask + 1;
    map->entries = calloc(newsize, sizeof(*map->entries));
    if (!map->entries) {
        map->entries = oldtab;
        return false;
    }

    map->mask = newsize - 1;

    if (!oldtab) {
        return true;
    }

    for (HashMapEntry *e = oldtab, *newe; e < oldend; e++) {
        if (e->key && e->key != tombstone) {
            for (size_t i = e->hash, j = 1; ; i += j++) {
                newe = map->entries + (i & map->mask);
                if (!newe->key) {
                    break;
                }
            }
            *newe = *e;
        }
    }

    free(oldtab);
    return true;
}

bool hashmap_init(HashMap *map, size_t capacity)
{
    *map = (HashMap) {
        .entries = NULL,
        .mask = 0,
        .count = 0,
    };
    return resize(map, capacity);
}

void hashmap_free(HashMap *map)
{
    free(map->entries);
}

static void hashmap_sanity_check(const HashMap *map)
{
    BUG_ON(!map);
    BUG_ON(!map->entries);
    BUG_ON(map->mask + 1 < MIN_SIZE);
    BUG_ON(map->count > map->mask);
}

HashMapEntry *hashmap_find(const HashMap *map, const char *key)
{
    hashmap_sanity_check(map);
    size_t hash = fnv_1a_hash(key, strlen(key));
    HashMapEntry *e;
    for (size_t i = hash, j = 1; ; i += j++) {
        e = map->entries + (i & map->mask);
        if (!e->key) {
            break;
        }
        if (e->key == tombstone) {
            continue;
        }
        if (e->hash == hash && streq(e->key, key)) {
            break;
        }
    }
    return e->key ? e : NULL;
}

void *hashmap_remove(HashMap *map, const char *key)
{
    HashMapEntry *e = hashmap_find(map, key);
    if (!e) {
        return NULL;
    }

    free(e->key);
    e->key = tombstone;
    map->count--;
    return e->value;
}

bool hashmap_insert(HashMap *map, char *key, void *value)
{
    hashmap_sanity_check(map);
    size_t hash = fnv_1a_hash(key, strlen(key));
    HashMapEntry *e;
    for (size_t i = hash, j = 1; ; i += j++) {
        e = map->entries + (i & map->mask);
        if (!e->key || e->key == tombstone) {
            break;
        }
        if (e->hash == hash && streq(e->key, key)) {
            // Don't replace existing values
            return false;
        }
    }

    e->key = key;
    e->value = value;
    e->hash = hash;

    if (++map->count > map->mask - (map->mask / 4)) {
        if (unlikely(!resize(map, 2 * map->count))) {
            map->count--;
            e->key = NULL;
            return false;
        }
    }

    return true;
}

bool hashmap_next(const HashMap *map, HashMapIter *iter)
{
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
