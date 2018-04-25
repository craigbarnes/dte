#include "xmalloc.h"
#include "common.h"

#define MALLOC_FAIL(msg) malloc_fail(__FILE__, __LINE__, __func__, (msg))

NORETURN static void malloc_fail (
    const char *const file,
    const int line,
    const char *const func,
    const char *const msg
) {
    char buf[256];
    term_cleanup();
    snprintf(buf, sizeof buf, "\n%s:%d: %s: %s\n", file, line, func, msg);
    fputs(buf, stderr);
    abort();
}

size_t size_multiply(size_t a, size_t b)
{
    if (unsigned_mult_overflows(a, b)) {
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
    if (unlikely(ptr == NULL)) {
        MALLOC_FAIL(strerror(errno));
    }
    return ptr;
}

void *xcalloc(size_t size)
{
    BUG_ON(size == 0);
    void *ptr = calloc(1, size);
    if (unlikely(ptr == NULL)) {
        MALLOC_FAIL(strerror(errno));
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
    BUG_ON(size == 0);
    ptr = realloc(ptr, size);
    if (unlikely(ptr == NULL)) {
        MALLOC_FAIL(strerror(errno));
    }
    return ptr;
}

char *xstrdup(const char *str)
{
    char *s = strdup(str);
    if (unlikely(s == NULL)) {
        MALLOC_FAIL(strerror(errno));
    }
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
