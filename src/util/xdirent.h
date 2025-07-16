#ifndef UTIL_XDIRENT_H
#define UTIL_XDIRENT_H

#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include "macros.h"

NONNULL_ARGS WARN_UNUSED_RESULT
static inline DIR *xopendir(const char *path)
{
    DIR *dir;
    do {
        dir = opendir(path); // NOLINT(*-unsafe-functions)
    } while (!dir && errno == EINTR);
    return dir;
}

NONNULL_ARGS WARN_UNUSED_RESULT
static inline struct dirent *xreaddir(DIR *dir)
{
    struct dirent *ent;
    do {
        errno = 0;
        ent = readdir(dir); // NOLINT(*-unsafe-functions)
    } while (!ent && errno == EINTR);
    return ent;
}

NONNULL_ARGS
static inline int xclosedir(DIR *dir)
{
    // We don't handle EINTR in a similar fashion to xclose() here, because
    // closedir() frees `dir` and it's typically unnecessary anyway.
    return closedir(dir);
}

#endif
