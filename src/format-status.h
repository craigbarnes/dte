#ifndef FORMAT_STATUS_H
#define FORMAT_STATUS_H

#include "window.h"

struct formatter {
    char *buf;
    long size;
    long pos;
    bool separator;
    Window *win;
    const char *misc_status;
};

void sf_init(struct formatter *f, Window *win);
void sf_format(struct formatter *f, char *buf, long size, const char *format);

#endif
