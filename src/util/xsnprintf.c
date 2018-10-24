#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include "xsnprintf.h"
#include "../debug.h"

int xvsnprintf (
    char *restrict buf,
    size_t len,
    const char *restrict format,
    va_list ap
) {
    if (unlikely(len > INT_MAX)) {
        fatal_error(__func__, EOVERFLOW);
    }
    const int n = vsnprintf(buf, len, format, ap);
    if (unlikely(n < 0 || n >= (int)len)) {
        fatal_error(__func__, ERANGE);
    }
    return n;
}

int xsnprintf(char *restrict buf, size_t len, const char *restrict format, ...)
{
    va_list ap;
    va_start(ap, format);
    const int n = xvsnprintf(buf, len, format, ap);
    va_end(ap);
    return n;
}
