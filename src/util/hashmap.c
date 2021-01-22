#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"
#include "debug.h"
#include "hash.h"
#include "str-util.h"

char tombstone[16] = "TOMBSTONE";

enum {
    MIN_SIZE = 8
};

WARN_UNUSED_RESULT
static bool hashmap_resize(HashMap *map, size_t size)
{
    BUG_ON(size < MIN_SIZE);
    BUG_ON(size <= map->count);
    BUG_ON(!IS_POWER_OF_2(size));

    HashMapEntry *newtab = calloc(size, sizeof(*newtab));
    if (unlikely(!newtab)) {
        return false;
    }

    HashMapEntry *oldtab = map->entries;
    size_t oldlen = map->mask + 1;
    map->entries = newtab;
    map->mask = size - 1;

    if (!oldtab) {
        return true;
    }

    // Copy the entries to the new table
    for (const HashMapEntry *e = oldtab, *end = e + oldlen; e < end; e++) {
        if (!e->key || e->key == tombstone) {
            continue;
        }
        HashMapEntry *newe;
        for (size_t i = e->hash, j = 1; ; i += j++) {
            newe = newtab + (i & map->mask);
            if (!newe->key) {
                break;
            }
        }
        *newe = *e;
    }

    free(oldtab);
    return true;
}

WARN_UNUSED_RESULT
static bool hashmap_do_init(HashMap *map, size_t size)
{
    // Accommodate the 75% load factor in the table size, to allow
    // filling to the requested size without needing to resize()
    size += size / 3;

    if (unlikely(size < MIN_SIZE)) {
        size = MIN_SIZE;
    }

    // Round up the size to the next power of 2, to allow using simple
    // bitwise ops (instead of modulo) to wrap the hash value and also
    // to allow quadratic probing with triangular numbers
    size = round_size_to_next_power_of_2(size);
    if (unlikely(size == 0)) {
        errno = EOVERFLOW;
        return false;
    }

    *map = (HashMap)HASHMAP_INIT;
    return hashmap_resize(map, size);
}

void hashmap_init(HashMap *map, size_t size)
{
    if (unlikely(!hashmap_do_init(map, size))) {
        fatal_error(__func__, errno);
    }
}

HashMapEntry *hashmap_find(const HashMap *map, const char *key)
{
    if (unlikely(!map->entries)) {
        return NULL;
    }

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

WARN_UNUSED_RESULT
static bool hashmap_do_insert(HashMap *map, char *key, void *value)
{
    if (unlikely(!map->entries)) {
        if (unlikely(!hashmap_do_init(map, 0))) {
            return false;
        }
    }

    size_t hash = fnv_1a_hash(key, strlen(key));
    HashMapEntry *e;
    for (size_t i = hash, j = 1; ; i += j++) {
        e = map->entries + (i & map->mask);
        if (!e->key || e->key == tombstone) {
            break;
        }
        if (e->hash == hash && streq(e->key, key)) {
            // Don't replace existing values
            errno = EINVAL;
            return false;
        }
    }

    e->key = key;
    e->value = value;
    e->hash = hash;

    if (++map->count > map->mask - (map->mask / 4)) {
        size_t new_size = (map->mask + 1) << 1;
        if (unlikely(new_size == 0)) {
            errno = EOVERFLOW;
            goto error;
        }
        if (unlikely(!hashmap_resize(map, new_size))) {
            goto error;
        }
    }

    return true;

error:
    map->count--;
    e->key = NULL;
    return false;
}

void hashmap_insert(HashMap *map, char *key, void *value)
{
    if (!hashmap_do_insert(map, key, value) && errno != EINVAL) {
        fatal_error(__func__, errno);
    }
}

// Remove all entries without freeing the table
void hashmap_clear(HashMap *map, FreeFunction free_value)
{
    if (unlikely(!map->entries)) {
        return;
    }

    size_t count = 0;
    for (HashMapIter it = hashmap_iter(map); hashmap_next(&it); count++) {
        free(it.entry->key);
        if (free_value) {
            free_value(it.entry->value);
        }
    }

    BUG_ON(count != map->count);
    size_t len = map->mask + 1;
    memset(map->entries, 0, len * sizeof(*map->entries));
    map->count = 0;
}

void hashmap_free(HashMap *map, FreeFunction free_value)
{
    hashmap_clear(map, free_value);
    free(map->entries);
    *map = (HashMap)HASHMAP_INIT;
}
