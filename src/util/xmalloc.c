#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xmalloc.h"
#include "../common.h"

#define CHECK_ALLOC(x) do { \
    if (unlikely((x) == NULL)) { \
        fail(__func__, errno); \
    } \
} while (0)

NORETURN COLD NONNULL_ARGS
static void fail(const char *msg, int err)
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

static int xvasprintf_(char **strp, const char *format, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, format, ap2);
    if (unlikely(n < 0)) {
        fail("vsnprintf", errno);
    }
    va_end(ap2);
    *strp = xmalloc(n + 1);
    int m = vsnprintf(*strp, n + 1, format, ap);
    BUG_ON(m != n);
    return n;
}

char *xvasprintf(const char *format, va_list ap)
{
    char *str;
    xvasprintf_(&str, format, ap);
    return str;
}

char *xasprintf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    char *str = xvasprintf(format, ap);
    va_end(ap);
    return str;
}
