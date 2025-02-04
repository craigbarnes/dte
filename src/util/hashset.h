#ifndef UTIL_HASHSET_H
#define UTIL_HASHSET_H

#include <stdbool.h>
#include <stddef.h>
#include "macros.h"

// A container type for holding a set of strings, using hashing for
// primary lookups and separate chaining for collision resolution.
typedef struct {
    struct HashSetEntry **table;
    size_t table_size;
    size_t nr_entries;
    size_t grow_at;
    size_t (*hash)(const unsigned char *str, size_t len);
    bool (*equal)(const void *s1, const void *s2, size_t n);
} HashSet;

typedef struct HashSetEntry {
    struct HashSetEntry *next;
    size_t str_len;
    char str[];
} HashSetEntry;

typedef struct {
    const HashSet *set;
    const HashSetEntry *entry;
    size_t idx;
} HashSetIter;

static inline HashSetIter hashset_iter(const HashSet *set)
{
    return (HashSetIter){.set = set};
}

// Set `iter->entry` to the next active HashSetEntry in iter->set and
// return true, or return false when there are no remaining entries to
// visit. Note that `iter->entry` is only NULL prior to the first call.
static inline bool hashset_next(HashSetIter *iter)
{
    if (likely(iter->entry) && iter->entry->next) {
        // Traverse linked entries
        iter->entry = iter->entry->next;
        return true;
    }

    // Traverse table entries
    const HashSet *set = iter->set;
    for (size_t i = iter->idx, n = set->table_size; i < n; i++) {
        const HashSetEntry *e = set->table[i];
        if (e) {
            iter->entry = e;
            iter->idx = i + 1;
            return true;
        }
    }

    return false;
}

void hashset_init(HashSet *set, size_t initial_size, bool icase);
void hashset_free(HashSet *set);
HashSetEntry *hashset_get(const HashSet *set, const char *str, size_t str_len);
HashSetEntry *hashset_insert(HashSet *set, const char *str, size_t str_len);

#endif
