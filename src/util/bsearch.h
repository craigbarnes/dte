#ifndef UTIL_BSEARCH_H
#define UTIL_BSEARCH_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "debug.h"
#include "macros.h"

typedef int (*CompareFunction)(const void *key, const void *elem);
typedef int (*StringCompareFunction)(const char *key, const char *elem);

#define BSEARCH(key, a, cmp) bsearch(key, a, ARRAY_COUNT(a), sizeof(a[0]), cmp)
#define BSEARCH_IDX(key, a, cmp) bisearch_idx(key, a, ARRAY_COUNT(a), sizeof(a[0]), cmp)

#define CHECK_BSEARCH_ARRAY(a, field, cmp) do { \
    static_assert_offsetof(a[0], field, 0); \
    static_assert_incompatible_types(a[0].field, const char*); \
    static_assert_incompatible_types(a[0].field, char*); \
    static_assert_compatible_types(a[0].field[0], char); \
    check_bsearch_array ( \
        a, #a, "." #field, ARRAY_COUNT(a), sizeof(a[0]), sizeof(a[0].field), cmp \
    ); \
} while (0)

#define CHECK_BSEARCH_STR_ARRAY(a, cmp) do { \
    static_assert_incompatible_types(a[0], const char*); \
    static_assert_incompatible_types(a[0], char*); \
    static_assert_compatible_types(a[0][0], char); \
    check_bsearch_array(a, #a, "", ARRAY_COUNT(a), sizeof(a[0]), sizeof(a[0]), cmp); \
} while (0)

static inline void check_bsearch_array (
    const void *array_base,
    const char *array_name,
    const char *name_field_name,
    size_t array_length,
    size_t array_element_size,
    size_t name_size,
    StringCompareFunction cmp
) {
    BUG_ON(!array_base);
    BUG_ON(!array_name);
    BUG_ON(!name_field_name);
    BUG_ON(name_field_name[0] != '\0' && name_field_name[0] != '.');
    BUG_ON(array_length == 0);
    BUG_ON(array_element_size == 0);
    BUG_ON(name_size == 0);
    BUG_ON(!cmp);

#if DEBUG >= 1
    const char *first_name = array_base;

    for (size_t i = 0; i < array_length; i++) {
        const char *curr_name = first_name + (i * array_element_size);
        // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
        if (curr_name[0] == '\0') {
            BUG("Empty string at %s[%zu]%s", array_name, i, name_field_name);
        }
        // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
        if (curr_name[name_size - 1] != '\0') {
            BUG("String sentinel missing from %s[%zu]%s", array_name, i, name_field_name);
        }

        // Skip sort order check for index 0; there's no prev_name to compare
        if (i == 0) {
            continue;
        }

        const char *prev_name = curr_name - array_element_size;
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
