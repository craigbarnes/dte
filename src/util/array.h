#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include <stddef.h>
#include "macros.h"
#include "ptr-array.h"
#include "str-util.h"

#define COLLECT_STRINGS(a, ptrs, prefix) \
    collect_strings_from_flat_array(*a, ARRAYLEN(a), sizeof(a[0]), ptrs, prefix)

#define STR_TO_ENUM(a, str, off, nfval) \
    str_to_enum(*a, ARRAYLEN(a), sizeof(a[0]), str, off, nfval)

// Attempt to find `str` in a "flat" array of strings (i.e. an array of
// type `const char[nr_elements][element_size]`) starting at `base`.
// If found, return the index plus `return_offset`, otherwise return
// `not_found_val`.
static inline int str_to_enum (
    const char *base,
    size_t nr_elements,
    size_t element_size,
    const char *str,
    int return_offset,
    int not_found_val
) {
    const char *entry = base;
    for (size_t i = 0; i < nr_elements; i++, entry += element_size) {
        if (streq(str, entry)) {
            return i + return_offset;
        }
    }
    return not_found_val;
}

void collect_strings_from_flat_array (
    const char *base,
    size_t nr_elements,
    size_t element_len,
    PointerArray *a,
    const char *prefix
);

#endif
