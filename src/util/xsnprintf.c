#include <errno.h>
#include <stdio.h>
#include "xsnprintf.h"
#include "debug.h"

/*
Note: ISO C doesn't require vsnprintf(3) to set errno on failure, but
POSIX does:

> If an output error was encountered, these functions shall return
> a negative value [CX] and set errno to indicate the error.

 - https://pubs.opengroup.org/onlinepubs/9699919799/functions/vfprintf.html
 - https://pubs.opengroup.org/onlinepubs/9699919799/functions/snprintf.html

The mandated errors of particular interest are:

 - [EOVERFLOW] The value of n is greater than INT_MAX
 - [EOVERFLOW] The value to be returned is greater than INT_MAX
*/

size_t xvsnprintf(char *restrict s, size_t n, const char *restrict fmt, va_list v)
{
    int ret = vsnprintf(s, n, fmt, v);
    if (unlikely(ret < 0)) {
        fatal_error(__func__, errno);
    }
    if (unlikely(ret >= (int)n)) {
        // Output truncated (insufficient buffer space)
        fatal_error(__func__, ENOBUFS);
    }
    return (size_t)ret;
}

size_t xsnprintf(char *restrict s, size_t n, const char *restrict fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    size_t ret = xvsnprintf(s, n, fmt, v);
    va_end(v);
    return ret;
}
