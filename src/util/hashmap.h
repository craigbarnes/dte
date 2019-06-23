#ifndef UTIL_HASHMAP_H
#define UTIL_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct HashMapEntry {
    struct HashMapEntry *next;
    uint32_t hash;
} HashMapEntry;

typedef int (*HashMapCompareFunction) (
    const void *hashmap_cmp_fn_data,
    const void *entry,
    const void *entry_or_key,
    const void *keydata
);

typedef struct {
    HashMapEntry **table;
    HashMapCompareFunction cmpfn;
    const void *cmpfn_data;
    unsigned int nr_entries;
    unsigned int tablesize;
    unsigned int grow_at;
    unsigned int shrink_at;
} HashMap;

typedef struct {
    HashMap *map;
    HashMapEntry *next;
    unsigned int tablepos;
} HashMapIterator;

void hashmap_init (
    HashMap *map,
    HashMapCompareFunction equals_function,
    const void *equals_function_data,
    unsigned int initial_size
);

void hashmap_free(HashMap *map, bool free_entries);

static inline void hashmap_entry_init(void *entry, uint32_t hash)
{
    HashMapEntry *e = entry;
    e->hash = hash;
    e->next = NULL;
}

/*
 * Returns the hashmap entry for the specified key, or NULL if not found.
 *
 * `map` is the HashMap structure.
 *
 * `key` is a user data structure that starts with a HashMapEntry that has at
 * least been initialized with the proper hash code (via `hashmap_entry_init`).
 *
 * `keydata` is a data structure that holds just enough information to check
 * for equality to a given entry.
 *
 * If the key data is variably-sized (e.g. a flexible array member) or quite
 * large, it's undesirable to create a full-fledged entry structure on the heap
 * and copy all the key data into the structure.
 *
 * In this case, the `keydata` parameter can be used to pass the key data
 * directly to the comparison function, and the `key` parameter can be a
 * stripped-down, fixed-size entry structure allocated on the stack.
 *
 * If an entry with matching hash code is found, `key` and `keydata` are passed
 * to `HashMapCompareFunction` to decide whether the entry matches the key.
 */
void *hashmap_get (
    const HashMap *map,
    const void *key,
    const void *keydata
);

/*
 * Returns the next equal hashmap entry, or NULL if not found. This can be
 * used to iterate over duplicate entries (see `hashmap_add`).
 *
 * `map` is the HashMap structure.
 * `entry` is the HashMapEntry to start the search from, obtained via a
 * previous call to `hashmap_get` or `hashmap_get_next`.
 */
void *hashmap_get_next(const HashMap *map, const void *entry);

/*
 * Adds a hashmap entry. This allows to add duplicate entries (i.e.
 * separate values with the same key according to `HashMapCompareFunction`).
 *
 * `map` is the hashmap structure.
 * `entry` is the entry to add.
 */
void hashmap_add(HashMap *map, void *entry);

/*
 * Adds or replaces a hashmap entry. If the HashMap contains duplicate
 * entries equal to the specified entry, only one of them will be replaced.
 *
 * `map` is the HashMap structure.
 * `entry` is the entry to add or replace.
 * Returns the replaced entry, or NULL if not found (i.e. the entry was added).
 */
void *hashmap_put(HashMap *map, void *entry);

/*
 * Removes a hashmap entry matching the specified key. If the hashmap contains
 * duplicate entries equal to the specified key, only one of them will be
 * removed. Returns the removed entry, or NULL if not found.
 *
 * Arguments are the same as in `hashmap_get`.
 */
void *hashmap_remove (
    HashMap *map,
    const void *key,
    const void *keydata
);

void hashmap_iter_init(HashMap *map, HashMapIterator *iter);
void *hashmap_iter_next(HashMapIterator *iter);

#endif
