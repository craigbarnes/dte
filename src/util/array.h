#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include <stddef.h>
#include "macros.h"
#include "ptr-array.h"

#define COLLECT_STRINGS(s, a, prefix) \
    collect_strings_from_flat_array(*s, ARRAYLEN(s), sizeof(s[0]), a, prefix)

void collect_strings_from_flat_array (
    const char *base,
    size_t nr_elements,
    size_t element_len,
    PointerArray *a,
    const char *prefix
);

#endif
