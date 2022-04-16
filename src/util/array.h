#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include <stdbool.h>
#include <sys/types.h>
#include "macros.h"
#include "ptr-array.h"
#include "str-util.h"

#define static_assert_flat_string_array(a) do { \
    static_assert_incompatible_types(a[0], const char*); \
    static_assert_incompatible_types(a[0], char*); \
    static_assert_compatible_types(a[0][0], char); \
} while(0)

#define static_assert_flat_struct_array(a, field) do { \
    static_assert_offsetof(a[0], field, 0); \
    static_assert_incompatible_types(a[0].field, const char*); \
    static_assert_incompatible_types(a[0].field, char*); \
    static_assert_compatible_types(a[0].field[0], char); \
} while(0)

#define COLLECT_STRINGS(a, ptrs, prefix) do { \
    static_assert_flat_string_array(a); \
    collect_strings_from_flat_array(a[0], ARRAYLEN(a), sizeof(a[0]), ptrs, prefix); \
} while(0)

#define COLLECT_STRING_FIELDS(a, field, ptrs, prefix) do { \
    static_assert_flat_struct_array(a, field); \
    collect_strings_from_flat_array(a[0].field, ARRAYLEN(a), sizeof(a[0]), ptrs, prefix); \
} while (0)

#define STR_TO_ENUM_WITH_OFFSET(str, a, nfval, off) \
    str_to_enum(str, a[0], ARRAYLEN(a), sizeof(a[0]), off, nfval)

#define STR_TO_ENUM(str, a, nfval) \
    STR_TO_ENUM_WITH_OFFSET(str, a, nfval, 0)

// This is somewhat similar to lfind(3), but returning an index
// instead of a pointer and specifically for arrays of type
// `const char[nmemb][size]`
static inline ssize_t find_str_idx (
    const char *str,
    const char *base,
    size_t nmemb,
    size_t size,
    bool (*equal)(const char *s1, const char *s2)
) {
    const char *entry = base;
    for (size_t i = 0; i < nmemb; i++, entry += size) {
        if (equal(str, entry)) {
            return i;
        }
    }
    return -1;
}

// Attempt to find `str` in a flat array of strings starting at `base`.
// If found, return the index plus `return_offset`, otherwise return
// `not_found_val`.
static inline int str_to_enum (
    const char *str,
    const char *base,
    size_t nmemb,
    size_t size,
    int return_offset,
    int not_found_val
) {
    ssize_t idx = find_str_idx(str, base, nmemb, size, streq);
    return (idx >= 0) ? idx + return_offset : not_found_val;
}

void collect_strings_from_flat_array (
    const char *base,
    size_t nr_elements,
    size_t element_len,
    PointerArray *a,
    const char *prefix
);

#endif
