#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xmalloc.h"
#include "../common.h"

#define CHECK_ALLOC(x) do { \
    if (unlikely((x) == NULL)) { \
        malloc_fail(__func__, errno); \
    } \
} while (0)

NORETURN COLD NONNULL_ARGS
static void malloc_fail(const char *msg, int err)
{
    term_cleanup();
    errno = err;
    perror(msg);
    abort();
}

size_t size_multiply(size_t a, size_t b)
{
    if (unlikely(unsigned_mult_overflows(a, b))) {
        char buf[256];
        term_cleanup();
        snprintf(buf, sizeof buf, "\nsize_t overflow: %zu * %zu\n", a, b);
        fputs(buf, stderr);
        exit(1);
    }
    return a * b;
}

void *xmalloc(size_t size)
{
    BUG_ON(size == 0);
    void *ptr = malloc(size);
    CHECK_ALLOC(ptr);
    return ptr;
}

void *xcalloc(size_t size)
{
    BUG_ON(size == 0);
    void *ptr = calloc(1, size);
    CHECK_ALLOC(ptr);
    return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
    BUG_ON(size == 0);
    ptr = realloc(ptr, size);
    CHECK_ALLOC(ptr);
    return ptr;
}

char *xstrdup(const char *str)
{
    char *s = strdup(str);
    CHECK_ALLOC(s);
    return s;
}

char *xstrcut(const char *str, size_t size)
{
    char *s = xmalloc(size + 1);
    memcpy(s, str, size);
    s[size] = '\0';
    return s;
}

void *xmemdup(const void *ptr, size_t size)
{
    BUG_ON(size == 0);
    void *buf = xmalloc(size);
    memcpy(buf, ptr, size);
    return buf;
}
