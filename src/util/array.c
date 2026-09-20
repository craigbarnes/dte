#include "array.h"
#include "log.h"
#include "str-util.h"
#include "xmalloc.h"

// This can be used to collect all prefix-matched strings from a "flat" array
// (i.e. an array of fixed-length char arrays; *not* pointers to char)
void collect_strings_from_flat_array (
    const char *base,
    size_t nr_elements,
    size_t element_len,
    PointerArray *a,
    StringView prefix
) {
    const char *end = base + (nr_elements * element_len);
    for (const char *str = base; str < end; str += element_len) {
        if (str_has_sv_prefix(str, prefix)) {
            ptr_array_append(a, xstrdup(str));
        }
    }
}

// Return bitflags corresponding to a set of comma-delimited substrings
// found in an array. For example, if the string is "str3,str7" and
// those 2 substrings are found at array[3] and array[7] respectively,
// the returned value will be `1 << 3 | 1 << 7`.
unsigned int str_to_bitflags (
    StringView str,
    const char *base, // Pointer to start of char[nstrs][size] array
    size_t nstrs,
    size_t size,
    bool tolerate_errors // Whether to ignore invalid substrings
) {
    unsigned int flags = 0;
    for (size_t pos = 0; pos < str.length; ) {
        StringView substr = get_delim(str, &pos, ',');
        ssize_t idx = find_strview_idx(substr, base, nstrs, size);
        if (unlikely(idx < 0)) {
            if (!tolerate_errors) {
                return 0;
            }
            LOG_WARNING("unrecognized flag string: '%.*s'", SV_FMT(substr));
            continue;
        }
        flags |= 1u << idx;
    }

    return flags;
}
