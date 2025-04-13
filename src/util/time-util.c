#include <errno.h>
#include "time-util.h"
#include "numtostr.h"

char *timespec_to_str(const struct timespec *ts, char buf[TIME_STR_BUFSIZE])
{
    if (unlikely(ts->tv_nsec < 0 || ts->tv_nsec >= NS_PER_SECOND)) {
        errno = EINVAL;
        return NULL;
    }

    struct tm tm;
    if (unlikely(!localtime_r(&ts->tv_sec, &tm))) {
        return NULL;
    }

    // Append date and time
    size_t i = strftime(buf, TIME_STR_BUFSIZE, "%F %T.", &tm);
    bool overflow = TIME_STR_BUFSIZE < i + sizeof("123456789 +0600");
    if (unlikely(i == 0 || overflow)) {
        errno = overflow ? ERANGE : errno;
        return NULL;
    }

    // Append nanoseconds
    i += buf_umax_to_str(ts->tv_nsec, buf + i);

    // Append timezone
    size_t zlen = strftime(buf + i, TIME_STR_BUFSIZE - i, " %z", &tm);
    return likely(zlen) ? buf : NULL;
}
