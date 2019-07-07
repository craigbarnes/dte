#ifndef SYNTAX_HASHSET_H
#define SYNTAX_HASHSET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../util/macros.h"

// This is a container type for holding a set of related strings.
// It uses hashing for primary lookups and separate chaining for
// collision resolution.
typedef struct {
    struct HashSetEntry **table;
    size_t table_size;
    size_t nr_entries;
    size_t grow_at;
    uint32_t (*hash)(const char *str, size_t len);
    bool (*equal)(const void *s1, const void *s2, size_t n);
} HashSet;

typedef struct HashSetEntry {
    struct HashSetEntry *next;
    size_t str_len;
    char str[];
} HashSetEntry;

void hashset_init(HashSet *set, size_t initial_size, bool icase);
void hashset_free(HashSet *set);
void hashset_add(HashSet *set, const char *str, size_t str_len);
void hashset_add_many(HashSet *set, char **strings, size_t nstrings);
bool hashset_contains(const HashSet *set, const char *str, size_t str_len);

#endif
