#include "array.h"
#include "str-util.h"
#include "xmalloc.h"

// This can be used to collect all prefix-matched strings from a "flat" array
// (i.e. an array of fixed-length char arrays; *not* pointers to char)
void collect_strings_from_flat_array (
    const char *base,
    size_t nr_elements,
    size_t element_len,
    PointerArray *a,
    const char *prefix
) {
    const char *end = base + (nr_elements * element_len);
    for (const char *str = base; str < end; str += element_len) {
        if (str_has_prefix(str, prefix)) {
            ptr_array_append(a, xstrdup(str));
        }
    }
}
