#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "checked-arith.h"
#include "xmalloc.h"
#include "../debug.h"

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

size_t round_size_to_next_power_of_2(size_t x)
{
    if (unlikely(x == 0)) {
        return 1;
    }
    x--;
    UNROLL_LOOP(8)
    for (size_t i = 1, n = sizeof(size_t) * CHAR_BIT; i < n; i <<= 1) {
        x |= x >> i;
    }
    if (unlikely(x == SIZE_MAX)) {
        fatal_error(__func__, EOVERFLOW);
    }
    return x + 1;
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

VPRINTF(2)
static int xvasprintf_(char **strp, const char *format, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, format, ap2);
    if (unlikely(n < 0)) {
        fatal_error("vsnprintf", EILSEQ);
    }
    va_end(ap2);
    *strp = xmalloc(n + 1);
    int m = vsnprintf(*strp, n + 1, format, ap);
    BUG_ON(m != n);
    return n;
}

VPRINTF(1)
static char *xvasprintf(const char *format, va_list ap)
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
