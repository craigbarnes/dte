#ifndef CMDLINE_H
#define CMDLINE_H

#include "ptr-array.h"
#include "gbuf.h"
#include "term.h"

struct cmdline {
	struct gbuf buf;
	long pos;
	int search_pos;
	char *search_text;
};

enum {
	CMDLINE_UNKNOWN_KEY,
	CMDLINE_KEY_HANDLED,
	CMDLINE_CANCEL,
};

#define CMDLINE(name) struct cmdline name = { GBUF_INIT, 0, -1, NULL }

void cmdline_clear(struct cmdline *c);
void cmdline_set_text(struct cmdline *c, const char *text);
int cmdline_handle_key(struct cmdline *c, struct ptr_array *history, int key);

#endif
