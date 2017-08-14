#ifndef FORMAT_STATUS_H
#define FORMAT_STATUS_H

#include "window.h"

typedef struct {
    char *buf;
    long size;
    long pos;
    bool separator;
    Window *win;
    const char *misc_status;
} Formatter;

void sf_init(Formatter *f, Window *win);
void sf_format(Formatter *f, char *buf, long size, const char *format);

#endif
