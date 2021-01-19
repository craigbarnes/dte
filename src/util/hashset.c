#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "hashset.h"
#include "ascii.h"
#include "debug.h"
#include "hash.h"
#include "str-util.h"
#include "xmalloc.h"

static void alloc_table(HashSet *set, size_t size)
{
    BUG_ON(size < 8);
    BUG_ON(!IS_POWER_OF_2(size));
    set->table_size = size;
    set->table = xnew0(HashSetEntry*, size);
    set->grow_at = size - (size / 4); // 75% load factor (size * 0.75)
}

void hashset_init(HashSet *set, size_t size, bool icase)
{
    if (size < 8) {
        size = 8;
    }

    // Accommodate the 75% load factor in the table size, to allow filling
    // the set to the requested size without needing to rehash()
    size += size / 3;

    // Round up the allocation to the next power of 2, to allow using
    // simple bitwise ops (instead of modulo) in get_slot()
    size = round_size_to_next_power_of_2(size);
    if (unlikely(size == 0)) {
        fatal_error(__func__, EOVERFLOW);
    }

    alloc_table(set, size);
    set->nr_entries = 0;

    if (icase) {
        set->hash = fnv_1a_hash_icase;
        set->equal = mem_equal_icase;
    } else {
        set->hash = fnv_1a_hash;
        set->equal = mem_equal;
    }
}

void hashset_free(HashSet *set)
{
    for (size_t i = 0, n = set->table_size; i < n; i++) {
        HashSetEntry *h = set->table[i];
        while (h) {
            HashSetEntry *next = h->next;
            free(h);
            h = next;
        }
    }
    free(set->table);
}

static size_t get_slot(const HashSet *set, const char *str, size_t str_len)
{
    const size_t hash = set->hash(str, str_len);
    return hash & (set->table_size - 1);
}

HashSetEntry *hashset_get(const HashSet *set, const char *str, size_t str_len)
{
    const size_t slot = get_slot(set, str, str_len);
    HashSetEntry *h = set->table[slot];
    while (h) {
        if (str_len == h->str_len && set->equal(str, h->str, str_len)) {
            return h;
        }
        h = h->next;
    }
    return NULL;
}

static void rehash(HashSet *set, size_t newsize)
{
    size_t oldsize = set->table_size;
    HashSetEntry **oldtable = set->table;
    alloc_table(set, newsize);
    for (size_t i = 0; i < oldsize; i++) {
        HashSetEntry *e = oldtable[i];
        while (e) {
            HashSetEntry *next = e->next;
            const size_t slot = get_slot(set, e->str, e->str_len);
            e->next = set->table[slot];
            set->table[slot] = e;
            e = next;
        }
    }
    free(oldtable);
}

HashSetEntry *hashset_add(HashSet *set, const char *str, size_t str_len)
{
    HashSetEntry *h = hashset_get(set, str, str_len);
    if (h) {
        return h;
    }

    const size_t slot = get_slot(set, str, str_len);
    h = xmalloc(sizeof(*h) + str_len + 1);
    h->next = set->table[slot];
    h->str_len = str_len;
    memcpy(h->str, str, str_len);
    h->str[str_len] = '\0';
    set->table[slot] = h;

    if (++set->nr_entries > set->grow_at) {
        size_t new_size = set->table_size << 1;
        if (unlikely(new_size == 0)) {
            fatal_error(__func__, EOVERFLOW);
        }
        rehash(set, new_size);
    }

    return h;
}

const void *mem_intern(const void *data, size_t len)
{
    static HashSet pool;
    if (unlikely(pool.table_size == 0)) {
        hashset_init(&pool, 32, false);
    }

    HashSetEntry *e = hashset_add(&pool, data, len);
    return e->str;
}
