#ifndef WBUF_H
#define WBUF_H

#include <sys/types.h>

typedef struct {
    size_t fill;
    int fd;
    char buf[8192];
} WriteBuffer;

#define WBUF_INIT { \
    .fill = 0, \
    .fd = -1 \
}

ssize_t wbuf_flush(WriteBuffer *wbuf);
ssize_t wbuf_write(WriteBuffer *wbuf, const char *buf, size_t count);
ssize_t wbuf_write_str(WriteBuffer *wbuf, const char *str);
ssize_t wbuf_write_ch(WriteBuffer *wbuf, char ch);

#endif
