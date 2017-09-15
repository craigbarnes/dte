#include <assert.h>
#include "xmalloc.h"
#include "common.h"

void *xmalloc(size_t size)
{
    void *ptr = malloc(size);
    assert(ptr != NULL);
    return ptr;
}

void *xcalloc(size_t size)
{
    void *ptr = calloc(1, size);
    assert(ptr != NULL);
    return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    assert(ptr != NULL);
    return ptr;
}

char *xstrdup(const char *str)
{
    char *s = strdup(str);
    assert(s != NULL);
    return s;
}

char *xstrcut(const char *str, size_t size)
{
    char *s = xmalloc(size + 1);
    memcpy(s, str, size);
    s[size] = 0;
    return s;
}

void *xmemdup(const void *ptr, size_t size)
{
    void *buf = xmalloc(size);
    memcpy(buf, ptr, size);
    return buf;
}
