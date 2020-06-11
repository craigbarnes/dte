#ifndef UTIL_XREADWRITE_H
#define UTIL_XREADWRITE_H

#include <sys/types.h>
#include "macros.h"

ssize_t xread(int fd, void *buf, size_t count) NONNULL_ARGS;
ssize_t xwrite(int fd, const void *buf, size_t count) NONNULL_ARGS;
int xopen(const char *path, int oflag, ...) NONNULL_ARG(1);
int xclose(int fd);

#endif
