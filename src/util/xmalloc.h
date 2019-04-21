#ifndef UTIL_XMALLOC_H
#define UTIL_XMALLOC_H

#include <stdarg.h>
#include <stddef.h>
#include "macros.h"

#define xnew(type, n) xmalloc(size_multiply(sizeof(type), (n)))
#define xnew0(type, n) xcalloc(size_multiply(sizeof(type), (n)))

#define xrenew(mem, n) do { \
    mem = xrealloc(mem, size_multiply(sizeof(*mem), (n))); \
} while (0)

void *xmalloc(size_t size) XMALLOC ALLOC_SIZE(1);
void *xcalloc(size_t size) XMALLOC ALLOC_SIZE(1);
void *xrealloc(void *ptr, size_t size) RETURNS_NONNULL ALLOC_SIZE(2);
char *xstrdup(const char *str) XMALLOC NONNULL_ARGS;
char *xstrndup(const char *str, size_t n) XMALLOC NONNULL_ARGS;
char *xstrdup_toupper(const char *str) XMALLOC NONNULL_ARGS;
char *xstrcut(const char *str, size_t size) XMALLOC NONNULL_ARGS;
void *xmemdup(const void *ptr, size_t size) NONNULL_ARGS RETURNS_NONNULL ALLOC_SIZE(2);
char *xvasprintf(const char *format, va_list ap) VPRINTF(1) XMALLOC;
char *xasprintf(const char *format, ...) PRINTF(1) XMALLOC;
size_t size_multiply(size_t a, size_t b);
size_t size_add(size_t a, size_t b);

// Round x up to a multiple of r (which *must* be a power of 2)
CONST_FN
static inline size_t ROUND_UP(size_t x, size_t r)
{
    r--;
    return (x + r) & ~r;
}

XMALLOC NONNULL_ARGS
static inline char *xstrslice(const char *str, size_t pos, size_t end)
{
    return xstrcut(str + pos, end - pos);
}

#endif
