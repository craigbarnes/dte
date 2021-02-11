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
static int hashmap_resize(HashMap *map, size_t size)
{
    BUG_ON(size < MIN_SIZE);
    BUG_ON(size <= map->count);
    BUG_ON(!IS_POWER_OF_2(size));

    HashMapEntry *newtab = calloc(size, sizeof(*newtab));
    if (unlikely(!newtab)) {
        return ENOMEM;
    }

    HashMapEntry *oldtab = map->entries;
    size_t oldlen = map->mask + 1;
    map->entries = newtab;
    map->mask = size - 1;
    map->tombstones = 0;

    if (!oldtab) {
        return 0;
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
    return 0;
}

WARN_UNUSED_RESULT
static int hashmap_do_init(HashMap *map, size_t size)
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
        return EOVERFLOW;
    }

    *map = (HashMap)HASHMAP_INIT;
    return hashmap_resize(map, size);
}

void hashmap_init(HashMap *map, size_t size)
{
    int err = hashmap_do_init(map, size);
    if (unlikely(err)) {
        fatal_error(__func__, err);
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
    map->tombstones++;
    return e->value;
}

WARN_UNUSED_RESULT
static int hashmap_do_insert(HashMap *map, char *key, void *value, void **old_value)
{
    int err = 0;
    if (unlikely(!map->entries)) {
        err = hashmap_do_init(map, 0);
        if (unlikely(err)) {
            return err;
        }
    }

    size_t hash = fnv_1a_hash(key, strlen(key));
    bool replacing_tombstone_or_existing_value = false;
    HashMapEntry *e;
    for (size_t i = hash, j = 1; ; i += j++) {
        e = map->entries + (i & map->mask);
        if (!e->key) {
            break;
        }
        if (e->key == tombstone) {
            replacing_tombstone_or_existing_value = true;
            BUG_ON(map->tombstones == 0);
            map->tombstones--;
            break;
        }
        if (unlikely(e->hash == hash && streq(e->key, key))) {
            replacing_tombstone_or_existing_value = true;
            // When a caller passes NULL as the "old_value" return param,
            // it implies there can be no existing entry with the same key
            // as the one to be inserted.
            BUG_ON(!old_value);
            BUG_ON(!e->value);
            *old_value = e->value;
            free(key);
            key = e->key;
            map->count--;
            break;
        }
    }

    const size_t max_load = map->mask - (map->mask / 4);
    e->key = key;
    e->value = value;
    e->hash = hash;
    map->count++;

    if (unlikely(map->count + map->tombstones > max_load)) {
        BUG_ON(replacing_tombstone_or_existing_value);
        size_t new_size = map->mask + 1;
        if (map->count > map->tombstones || new_size <= 256) {
            // Only increase the size of the table when the number of
            // real entries is higher than the number of tombstones.
            // If the number of real entries is lower, the table is
            // most likely being filled with tombstones from repeated
            // insert/remove churn; so we just rehash at the same size
            // to clean out the tombstones.
            new_size <<= 1;
            if (unlikely(new_size == 0)) {
                err = EOVERFLOW;
                goto error;
            }
        }
        err = hashmap_resize(map, new_size);
        if (unlikely(err)) {
            goto error;
        }
    }

    return 0;

error:
    map->count--;
    e->key = NULL;
    return err;
}

void hashmap_insert(HashMap *map, char *key, void *value)
{
    int err = hashmap_do_insert(map, key, value, NULL);
    if (unlikely(err)) {
        fatal_error(__func__, err);
    }
}

void *hashmap_insert_or_replace(HashMap *map, char *key, void *value)
{
    void *replaced_value = NULL;
    int err = hashmap_do_insert(map, key, value, &replaced_value);
    if (unlikely(err)) {
        fatal_error(__func__, err);
    }
    return replaced_value;
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
