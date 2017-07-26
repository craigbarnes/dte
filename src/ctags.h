#ifndef CTAGS_H
#define CTAGS_H

#include "libc.h"

struct tag_file {
	char *filename;
	char *buf;
	long size;
	time_t mtime;
};

struct tag {
	char *name;
	char *filename;
	char *pattern;
	char *member;
	char *typeref;
	int line;
	char kind;
	bool local;
};

bool next_tag(struct tag_file *tf, size_t *posp, const char *prefix, int exact, struct tag *t);
void free_tag(struct tag *t);

#endif
