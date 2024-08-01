#ifndef UTIL_STR_ARRAY_H
#define UTIL_STR_ARRAY_H

// Utility functions for working with null-terminated arrays of null-terminated
// strings, like e.g. the "argv" argument passed to main()

#include <stdbool.h>
#include <stdlib.h>
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
    for (size_t i = 0; strs[i]; i++) {
        if (str_has_prefix(strs[i], prefix)) {
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
    char **dst = xnew(char*, count + 1);
    for (size_t i = 0; i < count; i++) {
        dst[i] = xstrdup(src[i]);
    }
    dst[count] = NULL;
    return dst;
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
