#ifndef MISC_H
#define MISC_H

#include <stdbool.h>
#include <stddef.h>
#include "util/unicode.h"
#include "view.h"

typedef struct {
    char *buf;
    size_t len;
    bool is_lines;
} Clipboard;

void select_block(View *v);
void unselect(View *v);
void cut(Clipboard *clip, View *v, size_t len, bool is_lines);
void copy(Clipboard *clip, View *v, size_t len, bool is_lines);
void paste(Clipboard *clip, View *v, bool at_cursor);
void insert_text(View *v, const char *text, size_t size, bool move_after);
void delete_ch(View *v);
void erase(View *v);
void insert_ch(View *v, CodePoint ch);
void join_lines(View *v);
void clear_lines(View *v);
void delete_lines(View *v);
void new_line(View *v, bool above);
void format_paragraph(View *v, size_t text_width);
void change_case(View *v, char mode);

#endif
