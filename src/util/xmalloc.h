#ifndef UTIL_XMALLOC_H
#define UTIL_XMALLOC_H

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include "macros.h"

#define xnew(type, n) xmalloc(size_multiply(sizeof(type), (n)))
#define xnew0(type, n) xcalloc(size_multiply(sizeof(type), (n)))

#define xrenew(mem, n) do { \
    mem = xrealloc(mem, size_multiply(sizeof(*mem), (n))); \
} while (0)

#define xmemdup_literal(l) xmemdup(l, sizeof("" l ""))

void *xmalloc(size_t size) XMALLOC ALLOC_SIZE(1);
void *xcalloc(size_t size) XMALLOC ALLOC_SIZE(1);
void *xrealloc(void *ptr, size_t size) RETURNS_NONNULL ALLOC_SIZE(2);
char *xstrdup(const char *str) XSTRDUP;
char *xstrndup(const char *str, size_t n) XSTRDUP;
char *xstrdup_toupper(const char *str) XSTRDUP;
char *xstrcut(const char *str, size_t size) XSTRDUP;
char *xvasprintf(const char *format, va_list ap) VPRINTF(1) XMALLOC;
char *xasprintf(const char *format, ...) PRINTF(1) XMALLOC;
size_t size_multiply_(size_t a, size_t b);
size_t size_add(size_t a, size_t b);

static inline size_t size_multiply(size_t a, size_t b)
{
    // If either argument is 1, the multiplication can't overflow
    // and is thus safe to be inlined without checks.
    if (a == 1 || b == 1) {
        return a * b;
    }
    // Otherwise, emit a call to the checked implementation (which is
    // extern, because it may call fatal_error()).
    return size_multiply_(a, b);
}

NONNULL_ARGS_AND_RETURN ALLOC_SIZE(2)
static inline void *xmemdup(const void *ptr, size_t size)
{
    void *buf = xmalloc(size);
    memcpy(buf, ptr, size);
    return buf;
}

// Round x up to a multiple of r (which *must* be a power of 2)
static inline size_t ROUND_UP(size_t x, size_t r)
DIAGNOSE_IF(!IS_POWER_OF_2(r))
{
    r--;
    return (x + r) & ~r;
}

static inline size_t round_size_to_next_power_of_2(size_t x)
{
    x--;
    for (size_t i = 1, n = sizeof(size_t) * CHAR_BIT; i < n; i <<= 1) {
        x |= x >> i;
    }
    return x + 1;
}

XSTRDUP
static inline char *xstrslice(const char *str, size_t pos, size_t end)
{
    return xstrcut(str + pos, end - pos);
}

#endif
