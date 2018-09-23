#ifndef UTIL_XREADWRITE_H
#define UTIL_XREADWRITE_H

#include <sys/types.h>

ssize_t xread(int fd, void *buf, size_t count);
ssize_t xwrite(int fd, const void *buf, size_t count);

#endif
