#ifndef UTIL_XMALLOC_H
#define UTIL_XMALLOC_H

#include <stddef.h>
#include <string.h>
#include "arith.h"
#include "macros.h"

#define XMEMDUP(ptr) xmemdup(ptr, sizeof(*ptr))
#define xnew(type, n) xmalloc(size_multiply(sizeof(type), (n)))
#define xnew0(type, n) xcalloc(size_multiply(sizeof(type), (n)))

#define xrenew(mem, n) do { \
    mem = xreallocarray(mem, (n), sizeof(*mem)); \
} while (0)

void *xmalloc(size_t size) XMALLOC ALLOC_SIZE(1);
void *xcalloc(size_t size) XMALLOC ALLOC_SIZE(1);
void *xrealloc(void *ptr, size_t size) RETURNS_NONNULL ALLOC_SIZE(2);
char *xstrdup(const char *str) XSTRDUP;
char *xasprintf(const char *format, ...) PRINTF(1) XMALLOC;
size_t size_multiply_(size_t a, size_t b);
size_t size_add(size_t a, size_t b);

static inline size_t size_multiply(size_t a, size_t b)
{
    // If either argument is known at compile-time to be 1, the multiplication
    // can't overflow and is thus safe to be inlined without checks
    if ((IS_CT_CONSTANT(a) && a == 1) || (IS_CT_CONSTANT(b) && b == 1)) {
        return a * b;
    }
    // Otherwise, emit a call to the checked implementation
    return size_multiply_(a, b);
}

RETURNS_NONNULL ALLOC_SIZE(2, 3)
static inline void *xreallocarray(void *ptr, size_t nmemb, size_t size)
{
    return xrealloc(ptr, size_multiply(nmemb, size));
}

NONNULL_ARGS_AND_RETURN ALLOC_SIZE(2)
static inline void *xmemdup(const void *ptr, size_t size)
{
    return memcpy(xmalloc(size), ptr, size);
}

XSTRDUP
static inline char *xstrcut(const char *str, size_t size)
{
    char *s = xmalloc(size + 1);
    s[size] = '\0';
    return memcpy(s, str, size);
}

XSTRDUP
static inline char *xstrslice(const char *str, size_t pos, size_t end)
{
    return xstrcut(str + pos, end - pos);
}

XSTRDUP
static inline char *xstrjoin(const char *s1, const char *s2)
{
    size_t n1 = strlen(s1);
    size_t n2 = strlen(s2);
    char *joined = xmalloc(size_add(n1, n2 + 1));
    memcpy(joined, s1, n1);
    memcpy(joined + n1, s2, n2 + 1);
    return joined;
}

#endif
