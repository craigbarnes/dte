#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "xmalloc.h"
#include "checked-arith.h"
#include "debug.h"

static void *check_alloc(void *alloc)
{
    if (unlikely(alloc == NULL)) {
        fatal_error(__func__, ENOMEM);
    }
    return alloc;
}

size_t size_multiply_(size_t a, size_t b)
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
    return check_alloc(malloc(size));
}

void *xcalloc(size_t size)
{
    BUG_ON(size == 0);
    return check_alloc(calloc(1, size));
}

void *xrealloc(void *ptr, size_t size)
{
    BUG_ON(size == 0);
    return check_alloc(realloc(ptr, size));
}

char *xstrdup(const char *str)
{
    return check_alloc(strdup(str));
}

VPRINTF(1)
static char *xvasprintf(const char *format, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, format, ap2);
    va_end(ap2);
    if (unlikely(n < 0)) {
        goto error;
    } else if (unlikely(n >= INT_MAX)) {
        errno = EOVERFLOW;
        goto error;
    }

    char *str = malloc(n + 1);
    if (unlikely(!str)) {
        goto error;
    }

    int m = vsnprintf(str, n + 1, format, ap);
    if (unlikely(m < 0)) {
        goto error;
    }
    BUG_ON(m != n);
    return str;

error:
    fatal_error(__func__, errno);
}

char *xasprintf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    char *str = xvasprintf(format, ap);
    va_end(ap);
    return str;
}
