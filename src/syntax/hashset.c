#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "hashset.h"
#include "../util/ascii.h"
#include "../util/xmalloc.h"

#define FNV_BASE UINT32_C(2166136261)
#define FNV_PRIME UINT32_C(16777619)

static uint32_t fnv_1a_hash(const char *str, size_t len)
{
    uint32_t hash = FNV_BASE;
    while (len--) {
        uint32_t c = *str++;
        hash ^= c;
        hash *= FNV_PRIME;
    }
    return hash;
}

static uint32_t fnv_1a_hash_icase(const char *str, size_t len)
{
    uint32_t hash = FNV_BASE;
    while (len--) {
        uint32_t c = ascii_tolower(*str++);
        hash ^= c;
        hash *= FNV_PRIME;
    }
    return hash;
}

static int memcmp_strings(const char *s1, const char *s2, size_t n)
{
    return memcmp(s1, s2, n);
}

void hashset_init(HashSet *set, char **strings, size_t nstrings, bool icase)
{
    // Allocate table with 75% load factor (nstrings * 1.33)
    size_t table_size = ROUND_UP(size_add(nstrings, nstrings / 3), 8);
    HashSetEntry **table = xnew0(HashSetEntry*, table_size);
    set->table_size = table_size;
    set->table = table;

    if (icase) {
        set->hash = fnv_1a_hash_icase;
        set->compare = strncasecmp;
    } else {
        set->hash = fnv_1a_hash;
        set->compare = memcmp_strings;
    }

    for (size_t i = 0; i < nstrings; i++) {
        const char *str = strings[i];
        const size_t str_len = strlen(str);
        const uint32_t hash = set->hash(str, str_len);
        const size_t slot = (size_t)hash % table_size;
        HashSetEntry *h = xmalloc(sizeof(HashSetEntry) + str_len);
        h->next = table[slot];
        h->str_len = str_len;
        memcpy(h->str, str, str_len);
        table[slot] = h;
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

bool hashset_contains(HashSet *set, const char *str, size_t str_len)
{
    uint32_t hash = set->hash(str, str_len);
    HashSetEntry *h = set->table[hash % set->table_size];
    while (h) {
        if (str_len == h->str_len && !set->compare(str, h->str, str_len)) {
            return true;
        }
        h = h->next;
    }
    return false;
}
