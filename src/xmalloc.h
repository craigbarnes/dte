#ifndef XMALLOC_H
#define XMALLOC_H

#include <stddef.h>
#include "macros.h"

#define xnew(type, n) (type *)xmalloc(sizeof(type) * (n))
#define xnew0(type, n) (type *)xcalloc(sizeof(type) * (n))

#define xrenew(mem, n) do { \
    mem = xrealloc(mem, sizeof(*mem) * (n)); \
} while (0)

void *xmalloc(size_t size) MALLOC RETURNS_NONNULL;
void *xcalloc(size_t size) MALLOC RETURNS_NONNULL;
void *xrealloc(void *ptr, size_t size) RETURNS_NONNULL;
char *xstrdup(const char *str) MALLOC NONNULL_ARGS RETURNS_NONNULL;
char *xstrcut(const char *str, size_t size) MALLOC NONNULL_ARGS RETURNS_NONNULL;
void *xmemdup(const void *ptr, size_t size) NONNULL_ARGS RETURNS_NONNULL;

static inline MALLOC NONNULL_ARGS RETURNS_NONNULL char *xstrslice (
    const char *str,
    size_t pos,
    size_t end
) {
    return xstrcut(str + pos, end - pos);
}

#endif
