#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include <stdbool.h>
#include <sys/types.h>
#include "macros.h"
#include "ptr-array.h"
#include "str-util.h"

#define COLLECT_STRINGS(a, ptrs, prefix) \
    collect_strings_from_flat_array(*a, ARRAYLEN(a), sizeof(a[0]), ptrs, prefix)

#define STR_TO_ENUM(a, str, off, nfval) \
    str_to_enum(str, *a, ARRAYLEN(a), sizeof(a[0]), off, nfval)

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
