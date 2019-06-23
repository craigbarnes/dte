#include <stdlib.h>
#include "hashmap.h"
#include "xmalloc.h"

#define HASHMAP_INITIAL_SIZE 64
#define HASHMAP_RESIZE_BITS 2
#define HASHMAP_LOAD_FACTOR 80 // percent

static void alloc_table(HashMap *map, unsigned int size)
{
    map->tablesize = size;
    map->table = xnew0(HashMapEntry*, size);

    map->grow_at = (unsigned int) ((uint64_t) size * HASHMAP_LOAD_FACTOR / 100);
    if (size <= HASHMAP_INITIAL_SIZE) {
        map->shrink_at = 0;
    } else {
        map->shrink_at = map->grow_at / ((1U << HASHMAP_RESIZE_BITS) + 1);
    }
}

static int entry_equals (
    const HashMap *map,
    const HashMapEntry *e1,
    const HashMapEntry *e2,
    const void *keydata
) {
    return
        (e1 == e2)
        || (
            e1->hash == e2->hash
            && !map->cmpfn(map->cmpfn_data, e1, e2, keydata)
        );
}

static unsigned int bucket(const HashMap *map, const HashMapEntry *key)
{
    return key->hash & (map->tablesize - 1);
}

static void rehash(HashMap *map, unsigned int newsize)
{
    unsigned int oldsize = map->tablesize;
    HashMapEntry **oldtable = map->table;
    alloc_table(map, newsize);
    for (unsigned int i = 0; i < oldsize; i++) {
        HashMapEntry *e = oldtable[i];
        while (e) {
            HashMapEntry *next = e->next;
            unsigned int b = bucket(map, e);
            e->next = map->table[b];
            map->table[b] = e;
            e = next;
        }
    }
    free(oldtable);
}

static HashMapEntry **find_entry_ptr (
    const HashMap *map,
    const HashMapEntry *key,
    const void *keydata
) {
    HashMapEntry **e = &map->table[bucket(map, key)];
    while (*e && !entry_equals(map, *e, key, keydata)) {
        e = &(*e)->next;
    }
    return e;
}

void hashmap_init (
    HashMap *map,
    HashMapCompareFunction equals_function,
    const void *cmpfn_data,
    unsigned int initial_size
) {
    unsigned int size = HASHMAP_INITIAL_SIZE;

    memset(map, 0, sizeof(*map));
    map->cmpfn = equals_function;
    map->cmpfn_data = cmpfn_data;

    // calculate initial table size and allocate the table
    initial_size = (unsigned int) ((uint64_t) initial_size * 100
            / HASHMAP_LOAD_FACTOR);
    while (initial_size > size) {
        size <<= HASHMAP_RESIZE_BITS;
    }
    alloc_table(map, size);
}

void hashmap_free(HashMap *map, bool free_entries)
{
    if (!map || !map->table) {
        return;
    }
    if (free_entries) {
        HashMapIterator iter;
        HashMapEntry *e;
        hashmap_iter_init(map, &iter);
        while ((e = hashmap_iter_next(&iter))) {
            free(e);
        }
    }
    free(map->table);
    memset(map, 0, sizeof(*map));
}

void *hashmap_get(const HashMap *map, const void *key, const void *keydata)
{
    return *find_entry_ptr(map, key, keydata);
}

void *hashmap_get_next(const HashMap *map, const void *entry)
{
    HashMapEntry *e = ((HashMapEntry*)entry)->next;
    for (; e; e = e->next) {
        if (entry_equals(map, entry, e, NULL)) {
            return e;
        }
    }
    return NULL;
}

void hashmap_add(HashMap *map, void *entry)
{
    unsigned int b = bucket(map, entry);
    ((HashMapEntry*)entry)->next = map->table[b];
    map->table[b] = entry;

    if (++map->nr_entries > map->grow_at) {
        rehash(map, map->tablesize << HASHMAP_RESIZE_BITS);
    }
}

void *hashmap_remove(HashMap *map, const void *key, const void *keydata)
{
    HashMapEntry *old;
    HashMapEntry **e = find_entry_ptr(map, key, keydata);
    if (!*e) {
        return NULL;
    }

    old = *e;
    *e = old->next;
    old->next = NULL;

    if (--map->nr_entries < map->shrink_at) {
        rehash(map, map->tablesize >> HASHMAP_RESIZE_BITS);
    }

    return old;
}

void *hashmap_put(HashMap *map, void *entry)
{
    HashMapEntry *old = hashmap_remove(map, entry, NULL);
    hashmap_add(map, entry);
    return old;
}

void hashmap_iter_init(HashMap *map, HashMapIterator *iter)
{
    iter->map = map;
    iter->tablepos = 0;
    iter->next = NULL;
}

void *hashmap_iter_next(HashMapIterator *iter)
{
    HashMapEntry *current = iter->next;
    for (;;) {
        if (current) {
            iter->next = current->next;
            return current;
        }

        if (iter->tablepos >= iter->map->tablesize) {
            return NULL;
        }

        current = iter->map->table[iter->tablepos++];
    }
}
