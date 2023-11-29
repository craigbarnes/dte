#include <errno.h>
#include "time-util.h"
#include "debug.h"
#include "numtostr.h"

char *timespec_to_str(const struct timespec *ts, char *buf, size_t bufsize)
{
    if (unlikely(ts->tv_nsec >= NS_PER_SECOND)) {
        errno = EINVAL;
        return NULL;
    }

    struct tm tm;
    if (unlikely(!localtime_r(&ts->tv_sec, &tm))) {
        return NULL;
    }

    // Append date and time
    char *ptr = buf;
    size_t max = bufsize;
    size_t n = strftime(ptr, max, "%F %T.", &tm);
    ptr += n;
    max -= n;
    if (unlikely(n == 0 || max < 10)) {
        errno = ENOBUFS;
        return NULL;
    }

    // Append nanoseconds
    n = buf_umax_to_str(ts->tv_nsec, ptr);
    BUG_ON(n == 0 || n > 9);
    ptr += n;
    max -= n;

    // Append timezone
    n = strftime(ptr, max, " %z", &tm);
    if (unlikely(n == 0)) {
        errno = ENOBUFS;
        return NULL;
    }

    return buf;
}
