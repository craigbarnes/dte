#ifndef UTIL_TIME_UTIL_H
#define UTIL_TIME_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <time.h>
#include "debug.h"
#include "macros.h"

#define NS_PER_SECOND (1000000000L)
#define MS_PER_SECOND (1000L)
#define NS_PER_MS (1000000L)
#define US_PER_MS (1000L)

static inline const struct timespec *get_stat_mtime(const struct stat *st)
{
#if defined(__APPLE__)
    return &st->st_mtimespec;
#else
    return &st->st_mtim;
#endif
}

static inline void timespec_subtract (
    const struct timespec *lhs,
    const struct timespec *rhs,
    struct timespec *res
) {
    BUG_ON(lhs->tv_nsec > NS_PER_SECOND);
    BUG_ON(rhs->tv_nsec > NS_PER_SECOND);
    res->tv_sec = lhs->tv_sec - rhs->tv_sec;
    res->tv_nsec = lhs->tv_nsec - rhs->tv_nsec;
    if (res->tv_nsec < 0) {
        res->tv_sec--;
        res->tv_nsec += NS_PER_SECOND;
    }
}

static inline bool timespecs_equal(const struct timespec *a, const struct timespec *b)
{
    return a->tv_sec == b->tv_sec && a->tv_nsec == b->tv_nsec;
}

char *timespec_to_str(const struct timespec *ts, char *buf, size_t bufsize) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
