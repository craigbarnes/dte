#ifndef INDENT_H
#define INDENT_H

#include "libc.h"

struct indent_info {
	// size in bytes
	int bytes;

	// width in chars
	int width;

	// Number of whole indentation levels (depends on the indent-width option).
	int level;

	// Only spaces or tabs depending of expand-tab, indent-width and tab-width.
	// Note that "sane" line can contain spaces after tabs for alignment.
	bool sane;

	// The line is empty or contains only white space.
	bool wsonly;
};

char *make_indent(int width);
int brace_indent_inc(const char *line, unsigned int len);
char *get_indent_for_next_line(const char *line, unsigned int len);
void get_indent_info(const char *buf, int len, struct indent_info *info);
int use_spaces_for_indent(void);
int get_indent_level_bytes_left(void);
int get_indent_level_bytes_right(void);
char *alloc_indent(int count, int *sizep);

#endif
