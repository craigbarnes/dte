#ifndef UTIL_XREADWRITE_H
#define UTIL_XREADWRITE_H

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include "macros.h"

NONNULL_ARGS
static inline int xopen(const char *path, int flags, mode_t mode)
{
    int fd;
    do {
        fd = open(path, flags, mode);
    } while (fd < 0 && errno == EINTR);

    return fd;
}

ssize_t xread(int fd, void *buf, size_t count) NONNULL_ARGS;
ssize_t xwrite(int fd, const void *buf, size_t count);
int xclose(int fd);

#endif
