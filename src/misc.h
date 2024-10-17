#ifndef MISC_H
#define MISC_H

#include <stdbool.h>
#include <stddef.h>
#include "util/string-view.h"
#include "view.h"

void select_block(View *view);
void unselect(View *view);
void delete_ch(View *view);
void erase(View *view);
void join_lines(View *view, const char *delim, size_t delim_len);
void clear_lines(View *view, bool auto_indent);
void change_case(View *view, char mode);

bool line_has_opening_brace(StringView line);
bool line_has_closing_brace(StringView line);

#endif
