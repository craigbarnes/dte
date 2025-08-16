#ifndef UTIL_XSTRING_H
#define UTIL_XSTRING_H

// Several of the *mem*() wrapper functions below explicitly allow NULL
// pointer arguments when the corresponding length bound is 0. This is
// unlike the equivalent/underlying library functions (where it would
// ordinarily be undefined behavior) and is done in the spirit of the
// WG14 N3322 proposal.
// See also:
// • https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3322.pdf

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

NONNULL_ARG_IF_NONZERO_LENGTH(1, 3) NONNULL_ARG_IF_NONZERO_LENGTH(2, 3)
static inline bool mem_equal(const void *s1, const void *s2, size_t n)
{
    BUG_ON(n && (!s1 || !s2)); // See N3322 reference above
    return n == 0 || memcmp(s1, s2, n) == 0;
}

// Portable version of glibc/FreeBSD mempcpy(3)
NONNULL_ARG(1) RETURNS_NONNULL NONNULL_ARG_IF_NONZERO_LENGTH(2, 3)
static inline void *xmempcpy(void *restrict dest, const void *restrict src, size_t n)
{
    BUG_ON(n && !src); // See N3322 reference above
    return likely(n) ? (char*)memcpy(dest, src, n) + n : dest;
}

NONNULL_ARG(1) RETURNS_NONNULL
static inline void *xmempcpy2 (
    void *dest,
    const void *p1, size_t n1,
    const void *p2, size_t n2
) {
    return xmempcpy(xmempcpy(dest, p1, n1), p2, n2);
}

NONNULL_ARG(1) RETURNS_NONNULL
static inline void *xmempcpy3 ( // NOLINT(readability-function-size)
    void *dest,
    const void *p1, size_t n1,
    const void *p2, size_t n2,
    const void *p3, size_t n3
) {
    return xmempcpy(xmempcpy2(dest, p1, n1, p2, n2), p3, n3);
}

NONNULL_ARG(1) RETURNS_NONNULL
static inline void *xmempcpy4 ( // NOLINT(readability-function-size)
    void *dest,
    const void *p1, size_t n1,
    const void *p2, size_t n2,
    const void *p3, size_t n3,
    const void *p4, size_t n4
) {
    return xmempcpy(xmempcpy3(dest, p1, n1, p2, n2, p3, n3), p4, n4);
}

static inline bool mem_equal_icase(const void *p1, const void *p2, size_t n)
{
    if (n == 0) {
        return true; // See N3322 reference above
    }

    const char *s1 = p1;
    const char *s2 = p2;
    BUG_ON(!s1 || !s2);

    while (n--) {
        if (ascii_tolower(*s1++) != ascii_tolower(*s2++)) {
            return false;
        }
    }

    return true;
}

#endif
