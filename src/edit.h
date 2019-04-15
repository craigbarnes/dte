#ifndef EDIT_H
#define EDIT_H

#include <stdbool.h>
#include <stddef.h>
#include "util/unicode.h"

void select_block(void);
void unselect(void);
void cut(size_t len, bool is_lines);
void copy(size_t len, bool is_lines);
void insert_text(const char *text, size_t size);
void paste(bool at_cursor);
void delete_ch(void);
void erase(void);
void insert_ch(CodePoint ch);
void join_lines(void);
void shift_lines(int count);
void clear_lines(void);
void new_line(void);
void format_paragraph(size_t text_width);
void change_case(int mode);

#endif
