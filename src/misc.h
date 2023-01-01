#ifndef MISC_H
#define MISC_H

#include <stdbool.h>
#include <stddef.h>
#include "util/unicode.h"
#include "view.h"

void select_block(View *view);
void unselect(View *view);
void insert_text(View *view, const char *text, size_t size, bool move_after);
void delete_ch(View *view);
void erase(View *view);
void insert_ch(View *view, CodePoint ch);
void join_lines(View *view);
void clear_lines(View *view, bool auto_indent);
void delete_lines(View *view);
void new_line(View *view, bool above);
void format_paragraph(View *view, size_t text_width);
void change_case(View *view, char mode);

#endif
