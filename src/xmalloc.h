#ifndef XMALLOC_H
#define XMALLOC_H

#include <stdlib.h>

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define MALLOC __attribute__((__malloc__))
#define NONNULL __attribute__((nonnull))
#else
#define MALLOC
#define NONNULL
#endif

#define xnew(type, n) (type *)xmalloc(sizeof(type) * (n))
#define xnew0(type, n) (type *)xcalloc(sizeof(type) * (n))
#define xrenew(mem, n) do { \
                    mem = xrealloc(mem, sizeof(*mem) * (n)); \
                } while (0)

void *xmalloc(size_t size) MALLOC;
void *xcalloc(size_t size) MALLOC;
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *str) MALLOC NONNULL;
char *xstrcut(const char *str, size_t size) MALLOC NONNULL;
void *xmemdup(const void *ptr, size_t size) NONNULL;

static inline MALLOC NONNULL char *xstrslice(const char *str, size_t pos, size_t end)
{
    return xstrcut(str + pos, end - pos);
}

#endif
