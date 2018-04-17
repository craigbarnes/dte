#ifndef INDENT_H
#define INDENT_H

#include <stdbool.h>

typedef struct {
    // Size in bytes
    int bytes;

    // Width in chars
    int width;

    // Number of whole indentation levels (depends on the indent-width option)
    int level;

    // Only spaces or tabs depending on expand-tab, indent-width and tab-width.
    // Note that "sane" line can contain spaces after tabs for alignment.
    bool sane;

    // The line is empty or contains only white space
    bool wsonly;
} IndentInfo;

char *make_indent(int width);
char *get_indent_for_next_line(const char *line, unsigned int len);
void get_indent_info(const char *buf, int len, IndentInfo *info);
bool use_spaces_for_indent(void);
int get_indent_level_bytes_left(void);
int get_indent_level_bytes_right(void);
char *alloc_indent(int count, int *sizep);

#endif
