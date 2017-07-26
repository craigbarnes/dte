#ifndef FORMAT_STATUS_H
#define FORMAT_STATUS_H

#include "window.h"

struct formatter {
	char *buf;
	long size;
	long pos;
	bool separator;
	struct window *win;
	const char *misc_status;
};

void sf_init(struct formatter *f, struct window *win);
void sf_format(struct formatter *f, char *buf, long size, const char *format);

#endif
