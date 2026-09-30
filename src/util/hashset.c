#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "hashset.h"
#include "bit.h"
#include "debug.h"
#include "hash.h"
#include "xmalloc.h"

static void alloc_table(HashSet *set, size_t size)
{
    BUG_ON(size < 8);
    BUG_ON(!IS_POWER_OF_2(size));
    set->table_size = size;
    set->table = xcalloc(size, sizeof(set->table[0]));
    set->grow_at = size - (size / 4); // 75% load factor (size * 0.75)
}

HashSet hashset_new(size_t size, bool icase)
{
    size = MAX(size, 8);

    // Accommodate the 75% load factor in the table size, to allow filling
    // the set to the requested size without needing to rehash()
    size += size / 3;

    // Round up the allocation to the next power of 2, to allow using
    // simple bitwise ops (instead of modulo) in get_slot()
    size = next_pow2(size);
    FATAL_ERROR_ON(size == 0, EOVERFLOW);

    HashSet set;
    alloc_table(&set, size);
    set.nr_entries = 0;
    set.icase = icase;
    return set;
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

static size_t get_slot(const HashSet *set, StringView str)
{
    size_t hash = set->icase ? fnv_1a_hash_icase(str) : fnv_1a_hash(str);
    return hash & (set->table_size - 1);
}

HashSetEntry *hashset_get(const HashSet *set, StringView str)
{
    if (set->icase) {
        size_t slot = get_slot(set, str);
        for (HashSetEntry *h = set->table[slot]; h; h = h->next) {
            if (strview_equal_icase(str, string_view(h->str, h->str_len))) {
                return h;
            }
        }
    } else {
        size_t slot = get_slot(set, str);
        for (HashSetEntry *h = set->table[slot]; h; h = h->next) {
            if (strview_equal(str, string_view(h->str, h->str_len))) {
                return h;
            }
        }
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
            const size_t slot = get_slot(set, string_view(e->str, e->str_len));
            e->next = set->table[slot];
            set->table[slot] = e;
            e = next;
        }
    }

    free(oldtable);
}

HashSetEntry *hashset_insert(HashSet *set, StringView str)
{
    HashSetEntry *h = hashset_get(set, str);
    if (h) {
        return h;
    }

    const size_t slot = get_slot(set, str);
    h = xmalloc(xadd3(sizeof(*h), str.length, 1));
    h->next = set->table[slot];
    h->str_len = str.length;
    memcpy(h->str, str.data, str.length);
    h->str[str.length] = '\0';
    set->table[slot] = h;

    if (++set->nr_entries > set->grow_at) {
        size_t new_size = set->table_size << 1;
        FATAL_ERROR_ON(new_size == 0, EOVERFLOW);
        rehash(set, new_size);
    }

    return h;
}
