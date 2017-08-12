#ifndef WBUF_H
#define WBUF_H

#include <stdlib.h>

typedef struct {
    int fill;
    int fd;
    char buf[8192];
} WriteBuffer;

#define WBUF(name) WriteBuffer name = { .fill = 0, .fd = -1, }

int wbuf_flush(WriteBuffer *wbuf);
int wbuf_write(WriteBuffer *wbuf, const char *buf, size_t count);
int wbuf_write_str(WriteBuffer *wbuf, const char *str);
int wbuf_write_ch(WriteBuffer *wbuf, char ch);

#endif
