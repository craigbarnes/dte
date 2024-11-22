#ifndef UTIL_XMALLOC_H
#define UTIL_XMALLOC_H

#include <stddef.h>
#include <string.h>
#include "arith.h"
#include "macros.h"

#define XMEMDUP(ptr) xmemdup(ptr, sizeof(*ptr))
#define xnew(type, n) xmalloc(xmul(sizeof(type), (n)))
#define xnew0(type, n) xcalloc((n), sizeof(type))
#define xrenew(mem, n) xreallocarray(mem, (n), sizeof(*mem))

void *xmalloc(size_t size) XMALLOC ALLOC_SIZE(1);
void *xcalloc(size_t nmemb, size_t size) XMALLOC ALLOC_SIZE(1, 2);
void *xrealloc(void *ptr, size_t size) RETURNS_NONNULL WARN_UNUSED_RESULT ALLOC_SIZE(2);
char *xstrdup(const char *str) XSTRDUP;
char *xasprintf(const char *format, ...) PRINTF(1) XMALLOC;
size_t xmul_(size_t a, size_t b);
size_t xadd(size_t a, size_t b);

// Equivalent to `a * b`, but calling fatal_error() for arithmetic overflow.
// Note that if either operand is 2, adding 1 to the result can never
// overflow (often useful when doubling a buffer plus 1 extra byte). Similar
// observations can be made when multiplying by other powers of 2 (except 1)
// due to the equivalence with left shifting.
static inline size_t xmul(size_t a, size_t b)
{
    // If either argument is known at compile-time to be 1, the multiplication
    // can't overflow and is thus safe to be inlined without checks
    if ((IS_CT_CONSTANT(a) && a <= 1) || (IS_CT_CONSTANT(b) && b <= 1)) {
        return a * b; // GCOVR_EXCL_LINE
    }
    // Otherwise, emit a call to the checked implementation
    return xmul_(a, b);
}

RETURNS_NONNULL WARN_UNUSED_RESULT ALLOC_SIZE(2, 3)
static inline void *xreallocarray(void *ptr, size_t nmemb, size_t size)
{
    return xrealloc(ptr, xmul(nmemb, size));
}

NONNULL_ARGS_AND_RETURN ALLOC_SIZE(2)
static inline void *xmemdup(const void *ptr, size_t size)
{
    return memcpy(xmalloc(size), ptr, size);
}

// Return a null-terminated copy of the first `size` bytes of `str`
XSTRDUP
static inline char *xstrcut(const char *str, size_t size)
{
    char *s = xmalloc(size + 1);
    s[size] = '\0';
    return memcpy(s, str, size);
}

// Return a null-terminated copy of the substring between `pos` and `end`
XSTRDUP
static inline char *xstrslice(const char *str, size_t pos, size_t end)
{
    return xstrcut(str + pos, end - pos);
}

NONNULL_ARGS_AND_RETURN
static inline void *xmemjoin(const void *p1, size_t n1, const void *p2, size_t n2)
{
    char *joined = xmalloc(xadd(n1, n2));
    memcpy(joined, p1, n1);
    memcpy(joined + n1, p2, n2);
    return joined;
}

XSTRDUP
static inline char *xstrjoin(const char *s1, const char *s2)
{
    return xmemjoin(s1, strlen(s1), s2, strlen(s2) + 1);
}

#endif
