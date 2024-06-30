#include <errno.h>
#include <stdio.h>
#include "xsnprintf.h"
#include "debug.h"

/*
 * ISO C doesn't require vsnprintf(3) to set errno on failure, but
 * POSIX does:
 *
 * "If an output error was encountered, these functions shall return
 * a negative value and set errno to indicate the error."
 *
 * The mandated errors of particular interest are:
 *
 * - [EOVERFLOW] The value of n is greater than INT_MAX
 * - [EOVERFLOW] The value to be returned is greater than INT_MAX
 *
 * ISO C11 states:
 *
 * "The vsnprintf function returns the number of characters that would
 * have been written had n been sufficiently large, not counting the
 * terminating null character, or a negative value if an encoding error
 * occurred. Thus, the null-terminated output has been completely
 * written if and only if the returned value is nonnegative and less
 * than n."
 *
 * See also:
 *
 * - ISO C11 §7.21.6.12p3
 * - https://pubs.opengroup.org/onlinepubs/9699919799/functions/vsnprintf.html
 * - https://pubs.opengroup.org/onlinepubs/9699919799/functions/snprintf.html
 */
size_t xvsnprintf(char *restrict s, size_t n, const char *restrict fmt, va_list v)
{
    int r = vsnprintf(s, n, fmt, v);
    if (unlikely(r < 0 || r >= (int)n)) {
        fatal_error(__func__, (r < 0) ? errno : ENOBUFS);
    }
    return (size_t)r;
}

size_t xsnprintf(char *restrict s, size_t n, const char *restrict fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    size_t ret = xvsnprintf(s, n, fmt, v);
    va_end(v);
    return ret;
}
