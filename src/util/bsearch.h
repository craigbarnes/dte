#ifndef UTIL_BSEARCH_H
#define UTIL_BSEARCH_H

#include <stddef.h>
#include <string.h>
#include "debug.h"
#include "macros.h"

#define BSEARCH(key, a, cmp) bsearch(key, a, ARRAY_COUNT(a), sizeof(a[0]), cmp)

#if DEBUG >= 1 && defined(HAS_TYPEOF)
    #define CHECK_BSEARCH_ARRAY(a, field) do { \
        static_assert_compatible_types(a[0].field[0], char); \
        check_bsearch_array ( \
            a, #a, #field, \
            ARRAY_COUNT(a), \
            sizeof(a[0]), \
            offsetof(__typeof__(*a), field), \
            sizeof(a[0].field) \
        ); \
    } while (0)
#else
    #define CHECK_BSEARCH_ARRAY(arr, field)
#endif

#if DEBUG >= 1
    #define CHECK_BSEARCH_STR_ARRAY(a) do { \
        static_assert_compatible_types(a[0][0], char); \
        check_bsearch_array(a, #a, "", ARRAY_COUNT(a), sizeof(a[0]), 0, sizeof(a[0])); \
    } while (0)
#else
    #define CHECK_BSEARCH_STR_ARRAY(arr)
#endif

static inline void check_bsearch_array (
    const void *array_base,
    const char *array_name,
    const char *name_field_name,
    size_t array_length,
    size_t array_element_size,
    size_t name_offset,
    size_t name_size
) {
    BUG_ON(!array_base);
    BUG_ON(!array_name);
    BUG_ON(!name_field_name);
    BUG_ON(array_length == 0);
    BUG_ON(array_element_size == 0);
    BUG_ON(name_size == 0);

    const char *first_name = (const char*)array_base + name_offset;
    if (first_name[name_size - 1] != '\0') {
        BUG("String sentinel missing from %s[0].%s", array_name, name_field_name);
    }

    for (size_t i = 1; i < array_length; i++) {
        const char *curr_name = first_name + (i * array_element_size);
        const char *prev_name = curr_name - array_element_size;
        if (curr_name[name_size - 1] != '\0') {
            BUG("String sentinel missing from %s[%zu].%s", array_name, i, name_field_name);
        }
        if (strcmp(curr_name, prev_name) <= 0) {
            BUG (
                "String at %s[%zu].%s not in sorted order: \"%s\" (prev: \"%s\")",
                array_name, i, name_field_name,
                curr_name, prev_name
            );
        }
    }
}

#endif
