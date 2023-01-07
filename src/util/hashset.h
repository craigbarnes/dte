#ifndef UTIL_HASHSET_H
#define UTIL_HASHSET_H

#include <stdbool.h>
#include <stddef.h>
#include "macros.h"

// This is a container type for holding a set of related strings.
// It uses hashing for primary lookups and separate chaining for
// collision resolution.
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

void hashset_init(HashSet *set, size_t initial_size, bool icase);
void hashset_free(HashSet *set);
HashSetEntry *hashset_get(const HashSet *set, const char *str, size_t str_len);
HashSetEntry *hashset_add(HashSet *set, const char *str, size_t str_len);

#endif
