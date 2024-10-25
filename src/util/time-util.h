#ifndef UTIL_TIME_UTIL_H
#define UTIL_TIME_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <time.h>
#include "debug.h"
#include "macros.h"

#define TIME_STR_BUFSIZE (64) // sizeof("292271025015-12-01 23:59:00.999999999 +0400")
#define NS_PER_SECOND (1000000000L)
#define MS_PER_SECOND (1000L)
#define NS_PER_MS (1000000L)
#define US_PER_MS (1000L)

static inline const struct timespec *get_stat_mtime(const struct stat *st)
{
#if defined(__APPLE__)
    return &st->st_mtimespec;
#else
    // https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_stat.h.html#tag_13_61_03
    return &st->st_mtim;
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

static inline int timespec_cmp(const struct timespec *a, const struct timespec *b)
{
    time_t sa = a->tv_sec;
    time_t sb = b->tv_sec;
    long na = a->tv_nsec;
    long nb = b->tv_nsec;
    BUG_ON(na >= NS_PER_SECOND);
    BUG_ON(nb >= NS_PER_SECOND);

    // Example #1: (sa == sb) && (na == nb):  0 + 0 =  0
    // Example #2: (sa <  sb) && (na >  nb): -2 + 1 = -1
    int r = 0;
    r += (sa == sb) ? 0 : (sa < sb ? -2 : 2);
    r += (na == nb) ? 0 : (na < nb ? -1 : 1);
    return r;
}

static inline bool timespecs_equal(const struct timespec *a, const struct timespec *b)
{
    return a->tv_sec == b->tv_sec && a->tv_nsec == b->tv_nsec;
}

char *timespec_to_str(const struct timespec *ts, char *buf, size_t bufsize) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
