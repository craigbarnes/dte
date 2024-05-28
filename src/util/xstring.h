#ifndef UTIL_XSTRING_H
#define UTIL_XSTRING_H

#include <stdbool.h>
#include <string.h>
#include "ascii.h"
#include "debug.h"
#include "macros.h"

NONNULL_ARGS
static inline bool streq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static inline bool xstreq(const char *a, const char *b)
{
    return (a == b) || (a && b && streq(a, b));
}

// See: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3261.pdf
static inline bool mem_equal(const void *s1, const void *s2, size_t n)
{
    BUG_ON(n && (!s1 || !s2));
    return n == 0 || memcmp(s1, s2, n) == 0;
}

static inline bool mem_equal_icase(const void *p1, const void *p2, size_t n)
{
    if (n == 0) {
        return true;
    }

    const unsigned char *s1 = p1;
    const unsigned char *s2 = p2;
    BUG_ON(!s1 || !s2);

    while (n--) {
        if (ascii_tolower(*s1++) != ascii_tolower(*s2++)) {
            return false;
        }
    }

    return true;
}

#endif
