#ifndef UTIL_BSEARCH_H
#define UTIL_BSEARCH_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "array.h"
#include "debug.h"
#include "macros.h"

typedef int (*CompareFunction)(const void *key, const void *elem);
typedef int (*StringCompareFunction)(const char *key, const char *elem);

#define BSEARCH(key, a, cmp) bsearch(key, a, ARRAYLEN(a), sizeof(a[0]), cmp)
#define BSEARCH_IDX(key, a, cmp) bisearch_idx(key, a, ARRAYLEN(a), sizeof(a[0]), cmp)

#define CHECK_BSEARCH_ARRAY(a, field, cmp) do { \
    static_assert_flat_struct_array(a, field); \
    check_bsearch_array ( \
        a, #a, "." #field, ARRAYLEN(a), sizeof(a[0]), sizeof(a[0].field), cmp \
    ); \
} while (0)

#define CHECK_BSEARCH_STR_ARRAY(a, cmp) do { \
    static_assert_flat_string_array(a); \
    check_bsearch_array(a, #a, "", ARRAYLEN(a), sizeof(a[0]), sizeof(a[0]), cmp); \
} while (0)

static inline void check_bsearch_array (
    const void *base,
    const char *array_name,
    const char *name_field_name,
    size_t array_len,
    size_t elem_size,
    size_t name_size,
    StringCompareFunction cmp
) {
    BUG_ON(!cmp);
    check_array(base, array_name, name_field_name, array_len, elem_size, name_size);

#if DEBUG >= 1
    const char *first_name = base;
    for (size_t i = 1; i < array_len; i++) {
        const char *curr_name = first_name + (i * elem_size);
        const char *prev_name = curr_name - elem_size;
        if (cmp(curr_name, prev_name) <= 0) {
            BUG (
                "String at %s[%zu]%s not in sorted order: \"%s\" (prev: \"%s\")",
                array_name, i, name_field_name,
                curr_name, prev_name
            );
        }
    }
#endif
}

// Like bsearch(3), but returning the index of the element instead of
// a pointer to it (or -1 if not found)
static inline ssize_t bisearch_idx (
    const void *key,
    const void *base,
    size_t nmemb,
    size_t size,
    CompareFunction compare
) {
    const char *found = bsearch(key, base, nmemb, size, compare);
    if (!found) {
        return -1;
    }
    const char *char_base = base;
    return (size_t)(found - char_base) / size;
}

#endif
