#ifndef UTIL_XREADWRITE_H
#define UTIL_XREADWRITE_H

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include "errorcode.h"
#include "macros.h"

NONNULL_ARGS WARN_UNUSED_RESULT
static inline int xopen(const char *path, int flags, mode_t mode)
{
    int fd;
    do {
        fd = open(path, flags | O_NOCTTY, mode); // NOLINT(*-unsafe-functions)
    } while (fd < 0 && errno == EINTR);

    return fd;
}

ssize_t xread(int fd, void *buf, size_t count) NONNULL_ARGS WARN_UNUSED_RESULT;
ssize_t xwrite(int fd, const void *buf, size_t count) NONNULL_ARGS WARN_UNUSED_RESULT;
ssize_t xread_all(int fd, void *buf, size_t count) NONNULL_ARGS WARN_UNUSED_RESULT;
ssize_t xwrite_all(int fd, const void *buf, size_t count) WARN_UNUSED_RESULT;
SystemErrno xclose(int fd);

#endif
