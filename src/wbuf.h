#ifndef WBUF_H
#define WBUF_H

#include <stdlib.h>

struct wbuf {
	int fill;
	int fd;
	char buf[8192];
};

#define WBUF(name) struct wbuf name = { .fill = 0, .fd = -1, }

int wbuf_flush(struct wbuf *wbuf);
int wbuf_write(struct wbuf *wbuf, const char *buf, size_t count);
int wbuf_write_str(struct wbuf *wbuf, const char *str);
int wbuf_write_ch(struct wbuf *wbuf, char ch);

#endif
