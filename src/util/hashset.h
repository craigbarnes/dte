#ifndef UTIL_HASHSET_H
#define UTIL_HASHSET_H

#include <stddef.h>
#include <stdbool.h>
#include "macros.h"

// This is a container type for holding a *set* of related strings.
// It uses hashing for primary lookups and separate chaining for
// collision resolution. Only construct and query operations are
// implemented, since that's all the codebase currently requires.

typedef struct {
    struct HashSetEntry **table;
    size_t table_size;
    unsigned long (*hash)(const char *str, size_t size);
    int (*compare)(const char *s1, const char *s2, size_t n);
} HashSet;

typedef struct HashSetEntry {
    struct HashSetEntry *next;
    size_t str_len;
    char str[];
} HashSetEntry;

void hashset_init(HashSet *set, char **strings, size_t nstrings, bool icase);
void hashset_free(HashSet *set);
bool hashset_contains(HashSet *set, const char *str, size_t str_len);

#endif
