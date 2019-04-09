#ifndef UTIL_WBUF_H
#define UTIL_WBUF_H

#include <sys/types.h>
#include "macros.h"

typedef struct {
    size_t fill;
    int fd;
    char buf[8192];
} WriteBuffer;

#define WBUF_INIT { \
    .fill = 0, \
    .fd = -1 \
}

ssize_t wbuf_flush(WriteBuffer *wbuf) NONNULL_ARGS;
ssize_t wbuf_write(WriteBuffer *wbuf, const char *buf, size_t count) NONNULL_ARGS;
ssize_t wbuf_write_str(WriteBuffer *wbuf, const char *str) NONNULL_ARGS;
ssize_t wbuf_write_ch(WriteBuffer *wbuf, char ch) NONNULL_ARGS;
void wbuf_need_space(WriteBuffer *wbuf, size_t count) NONNULL_ARGS;

#endif
