#ifndef XMALLOC_H
#define XMALLOC_H

#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include "util/macros.h"

#define bitsizeof(x) (CHAR_BIT * sizeof(x))

#define maximum_unsigned_value_of_type(a) \
    (UINTMAX_MAX >> (bitsizeof(uintmax_t) - bitsizeof(a)))

#define unsigned_mult_overflows(a, b) \
    ((a) && (b) > maximum_unsigned_value_of_type(a) / (a))

#define xnew(type, n) xmalloc(size_multiply(sizeof(type), (n)))
#define xnew0(type, n) xcalloc(size_multiply(sizeof(type), (n)))

#define xrenew(mem, n) do { \
    mem = xrealloc(mem, size_multiply(sizeof(*mem), (n))); \
} while (0)

void *xmalloc(size_t size) XMALLOC ALLOC_SIZE(1);
void *xcalloc(size_t size) XMALLOC ALLOC_SIZE(1);
void *xrealloc(void *ptr, size_t size) RETURNS_NONNULL ALLOC_SIZE(2);
char *xstrdup(const char *str) XMALLOC NONNULL_ARGS;
char *xstrcut(const char *str, size_t size) XMALLOC NONNULL_ARGS;
void *xmemdup(const void *ptr, size_t size) NONNULL_ARGS RETURNS_NONNULL ALLOC_SIZE(2);
size_t size_multiply(size_t a, size_t b);

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
