#ifndef UTIL_TIME_UTIL_H
#define UTIL_TIME_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <time.h>
#include "debug.h"
#include "log.h"
#include "macros.h"

#define TIME_STR_BUFSIZE (64) // sizeof("292271025015-12-01 23:59:00.999999999 +0400")
#define NS_PER_SECOND (1000000000L)
#define MS_PER_SECOND (1000L)
#define NS_PER_MS (1000000L)
#define US_PER_MS (1000L)

static inline const struct timespec *get_stat_mtime(const struct stat *st)
{
#if !defined(__APPLE__)
    // https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_stat.h.html#tag_13_61_03:~:text=struct%20timespec%20st_mtim
    return &st->st_mtim;
#else
    // macOS doesn't conform to POSIX 2008 and uses its own, platform-specific
    // naming for the `struct stat` timespec member
    return &st->st_mtimespec;
#endif
}

static inline struct timespec timespec_subtract (
    const struct timespec *lhs,
    const struct timespec *rhs
) {
    BUG_ON(lhs->tv_nsec >= NS_PER_SECOND);
    BUG_ON(rhs->tv_nsec >= NS_PER_SECOND);
    time_t sec = lhs->tv_sec - rhs->tv_sec;
    long nsec = lhs->tv_nsec - rhs->tv_nsec;
    return (struct timespec) {
        .tv_sec = sec - (nsec < 0),
        .tv_nsec = nsec + (nsec < 0 ? NS_PER_SECOND : 0),
    };
}

static inline double timespec_to_fp_milliseconds(struct timespec ts)
{
    const double ms_per_s = MS_PER_SECOND;
    const double ns_per_ms = NS_PER_MS;
    return ((double)ts.tv_sec * ms_per_s) + ((double)ts.tv_nsec / ns_per_ms);
}

static inline bool timespecs_equal(const struct timespec *a, const struct timespec *b)
{
    return a->tv_sec == b->tv_sec && a->tv_nsec == b->tv_nsec;
}

// Convenience wrapper for clock_gettime(), with bool return and error logging
static inline bool xgettime(struct timespec *ts)
{
    bool r = (clock_gettime(CLOCK_MONOTONIC, ts) == 0);
    if (unlikely(!r)) {
        LOG_ERRNO("clock_gettime");
    }
    return r;
}

char *timespec_to_str(const struct timespec *ts, char buf[TIME_STR_BUFSIZE]) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
