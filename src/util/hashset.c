#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "hashset.h"
#include "ascii.h"
#include "xmalloc.h"

static unsigned long buf_hash(const char *str, size_t size)
{
    unsigned long hash = 0;
    for (size_t i = 0; i < size; i++) {
        unsigned long ch = str[i];
        hash = (hash << 5) - hash + ch;
    }
    return hash;
}

static unsigned long buf_hash_icase(const char *str, size_t size)
{
    unsigned long hash = 0;
    for (size_t i = 0; i < size; i++) {
        unsigned long ch = ascii_tolower(str[i]);
        hash = (hash << 5) - hash + ch;
    }
    return hash;
}

static int memcmp_strings(const char *s1, const char *s2, size_t n)
{
    return memcmp(s1, s2, n);
}

void hashset_init(HashSet *set, char **strings, size_t nstrings, bool icase)
{
    size_t table_size = nstrings + (nstrings / 4);
    if (unlikely(table_size < nstrings)) {
        // size_t overflow; fall back to using 100% load factor
        table_size = nstrings;
    }
    HashSetEntry **table = xnew0(HashSetEntry*, table_size);
    set->table_size = table_size;
    set->table = table;
    if (icase) {
        set->hash = buf_hash_icase;
        set->compare = strncasecmp;
    } else {
        set->hash = buf_hash;
        set->compare = memcmp_strings;
    }
    for (size_t i = 0; i < nstrings; i++) {
        const char *str = strings[i];
        const size_t str_len = strlen(str);
        unsigned long slot = set->hash(str, str_len) % table_size;
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
    unsigned long hash = set->hash(str, str_len);
    HashSetEntry *h = set->table[hash % set->table_size];
    while (h) {
        if (str_len == h->str_len && !set->compare(str, h->str, str_len)) {
            return true;
        }
        h = h->next;
    }
    return false;
}
