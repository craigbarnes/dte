#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "xmalloc.h"
#include "debug.h"
#include "xsnprintf.h"

static void *check_alloc(void *alloc)
{
    if (unlikely(alloc == NULL)) {
        fatal_error(__func__, ENOMEM);
    }
    return alloc;
}

// Like malloc(3), but calling fatal_error() on OOM and forbidding
// zero-sized allocations (thus never returning NULL)
void *xmalloc(size_t size)
{
    BUG_ON(size == 0);
    return check_alloc(malloc(size));
}

void *xcalloc(size_t nmemb, size_t size)
{
    if (__STDC_VERSION__ < 202311L) {
        // ISO C23 (§7.24.3.2) requires calloc() to check for integer
        // overflow in `nmemb * size`, but older C standards don't
        xmul(nmemb, size);
    }

    BUG_ON(nmemb == 0 || size == 0);
    return check_alloc(calloc(nmemb, size));
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

    if (unlikely(n < 0 || n == INT_MAX || n >= SIZE_MAX)) {
        fatal_error(__func__, n < 0 ? errno : EOVERFLOW);
    }

    char *str = xmalloc(n + 1);
    size_t m = xvsnprintf(str, n + 1, format, ap);
    BUG_ON(m != n);
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
