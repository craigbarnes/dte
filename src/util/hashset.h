#ifndef UTIL_HASHSET_H
#define UTIL_HASHSET_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "macros.h"
#include "string-view.h"

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

const void *mem_intern(const void *data, size_t len) NONNULL_ARGS_AND_RETURN;

static inline const char *str_intern(const char *str)
{
    return mem_intern(str, strlen(str));
}

static inline StringView strview_intern(const char *str)
{
    size_t len = strlen(str);
    return string_view(mem_intern(str, len), len);
}

#endif
