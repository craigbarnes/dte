#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "checked-arith.h"
#include "xmalloc.h"
#include "ascii.h"
#include "../debug.h"

#define CHECK_ALLOC(x) do { \
    if (unlikely((x) == NULL)) { \
        fatal_error(__func__, errno); \
    } \
} while (0)

size_t size_multiply(size_t a, size_t b)
{
    size_t result;
    if (unlikely(size_multiply_overflows(a, b, &result))) {
        fatal_error(__func__, EOVERFLOW);
    }
    return result;
}

size_t size_add(size_t a, size_t b)
{
    size_t result;
    if (unlikely(size_add_overflows(a, b, &result))) {
        fatal_error(__func__, EOVERFLOW);
    }
    return result;
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

char *xstrndup(const char *str, size_t n)
{
    char *s = strndup(str, n);
    CHECK_ALLOC(s);
    return s;
}

char *xstrdup_toupper(const char *str)
{
    const size_t len = strlen(str);
    char *upper_str = xmalloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        upper_str[i] = ascii_toupper(str[i]);
    }
    upper_str[len] = '\0';
    return upper_str;
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

VPRINTF(2)
static int xvasprintf_(char **strp, const char *format, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, format, ap2);
    if (unlikely(n < 0)) {
        fatal_error("vsnprintf", errno);
    }
    va_end(ap2);
    *strp = xmalloc(n + 1);
    int m = vsnprintf(*strp, n + 1, format, ap);
    DEBUG_VAR(m);
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

UNITTEST {
    size_t r = 0;
    DEBUG_VAR(r);
    BUG_ON(size_multiply_overflows(10, 20, &r));
    BUG_ON(r != 200);
    BUG_ON(size_multiply_overflows(0, 0, &r));
    BUG_ON(r != 0);
    BUG_ON(size_multiply_overflows(1, 0, &r));
    BUG_ON(r != 0);
    BUG_ON(size_multiply_overflows(0, 1, &r));
    BUG_ON(r != 0);
    BUG_ON(size_multiply_overflows(0, SIZE_MAX, &r));
    BUG_ON(r != 0);
    BUG_ON(size_multiply_overflows(SIZE_MAX, 0, &r));
    BUG_ON(r != 0);
    BUG_ON(size_multiply_overflows(1, SIZE_MAX, &r));
    BUG_ON(r != SIZE_MAX);
    BUG_ON(size_multiply_overflows(2, SIZE_MAX / 3, &r));
    BUG_ON(r != 2 * (SIZE_MAX / 3));
    BUG_ON(!size_multiply_overflows(SIZE_MAX, 2, &r));
    BUG_ON(!size_multiply_overflows(2, SIZE_MAX, &r));
    BUG_ON(!size_multiply_overflows(3, SIZE_MAX / 2, &r));
    BUG_ON(!size_multiply_overflows(32767, SIZE_MAX, &r));
    BUG_ON(!size_multiply_overflows(SIZE_MAX, SIZE_MAX, &r));
    BUG_ON(!size_multiply_overflows(SIZE_MAX, SIZE_MAX / 2, &r));
}
