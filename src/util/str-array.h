#ifndef UTIL_STR_ARRAY_H
#define UTIL_STR_ARRAY_H

// Utility functions for working with null-terminated arrays of null-terminated
// strings, like e.g. the "argv" argument passed to main()

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "macros.h"
#include "str-util.h"
#include "xmalloc.h"
#include "xstring.h"

static inline size_t string_array_length(char **strings)
{
    size_t n = 0;
    while (strings[n]) {
        n++;
    }
    return n;
}

static inline bool string_array_contains_prefix(char **strs, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    for (size_t i = 0; strs[i]; i++) {
        if (str_has_strn_prefix(strs[i], prefix, prefix_len)) {
            return true;
        }
    }
    return false;
}

static inline bool string_array_contains_str(char **strs, const char *str)
{
    for (size_t i = 0; strs[i]; i++) {
        if (streq(strs[i], str)) {
            return true;
        }
    }
    return false;
}

static inline char **copy_string_array(char **src, size_t count)
{
    char **dst = xmallocarray(count + 1, sizeof(*dst));
    for (size_t i = 0; i < count; i++) {
        dst[i] = xstrdup(src[i]);
    }
    dst[count] = NULL;
    return dst;
}

// Concatenates an array of strings into a fixed-size buffer, as a single,
// delimiter-separated string. This is separate from string_array_concat()
// only as a convenience for testing.
WARN_UNUSED_RESULT NONNULL_ARGS
static inline bool string_array_concat_ (
    char *buf, size_t bufsize,
    const char *const *strs, size_t nstrs,
    const char *delim, size_t delim_len
) {
    BUG_ON(bufsize == 0);
    const char *end = buf + bufsize;
    char *ptr = buf;
    buf[0] = '\0'; // Always null-terminate `buf`, even if `nstrs == 0`

    for (size_t i = 0; i < nstrs; i++) {
        bool last_iter = (i + 1 == nstrs);
        ptr = memccpy(ptr, strs[i], '\0', end - ptr);
        if (unlikely(!ptr || (!last_iter && ptr + delim_len > end))) {
            return false;
        }
        // Append `delim` (over the copied '\0'), if not on the last iteration
        ptr = last_iter ? ptr : xmempcpy(ptr - 1, delim, delim_len);
    }

    return true;
}

// Like string_array_concat_(), but asserting that it should never return
// false (i.e. because the required `bufsize` is known at compile-time)
static inline void string_array_concat (
    char *buf, size_t bufsize,
    const char *const *strs, size_t nstrs,
    const char *delim, size_t delim_len
) {
    bool r = string_array_concat_(buf, bufsize, strs, nstrs, delim, delim_len);
    BUG_ON(!r);
}

NONNULL_ARGS
static inline void free_string_array(char **strings)
{
    for (size_t i = 0; strings[i]; i++) {
        free(strings[i]);
    }
    free(strings);
}

#endif
