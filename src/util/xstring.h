#ifndef UTIL_XSTRING_H
#define UTIL_XSTRING_H

#include <stdbool.h>
#include <string.h>
#include "ascii.h"
#include "debug.h"
#include "macros.h"

// Return true if null-terminated strings a and b are identical
NONNULL_ARGS
static inline bool streq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

// Like streq(), but allowing one or both parameters to be NULL
static inline bool xstreq(const char *a, const char *b)
{
    return (a == b) || (a && b && streq(a, b));
}

// Like strrchr(3), but for use when `ch` is known to be present in
// `str` (e.g. when searching for '/' in absolute filenames)
NONNULL_ARGS_AND_RETURN
static inline const char *xstrrchr(const char *str, int ch)
{
    const char *ptr = strrchr(str, ch);
    BUG_ON(!ptr);
    return ptr;
}

// See: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3322.pdf
static inline bool mem_equal(const void *s1, const void *s2, size_t n)
{
    BUG_ON(n && (!s1 || !s2));
    return n == 0 || memcmp(s1, s2, n) == 0;
}

// Portable version of glibc/FreeBSD mempcpy(3)
NONNULL_ARGS_AND_RETURN
static inline void *xmempcpy(void *restrict dest, const void *restrict src, size_t n)
{
    memcpy(dest, src, n);
    return (char*)dest + n;
}

NONNULL_ARGS_AND_RETURN
static inline void *xmempcpy2 (
    void *dest,
    const void *p1, size_t n1,
    const void *p2, size_t n2
) {
    return xmempcpy(xmempcpy(dest, p1, n1), p2, n2);
}

NONNULL_ARGS_AND_RETURN
static inline void *xmempcpy3 (
    void *dest,
    const void *p1, size_t n1,
    const void *p2, size_t n2,
    const void *p3, size_t n3
) {
    return xmempcpy(xmempcpy2(dest, p1, n1, p2, n2), p3, n3);
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
