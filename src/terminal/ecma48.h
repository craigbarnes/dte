#ifndef TERMINAL_ECMA48_H
#define TERMINAL_ECMA48_H

#include "color.h"
#include "../util/string-view.h"

void ecma48_clear_to_eol(void);
void ecma48_move_cursor(int x, int y);
void ecma48_set_color(const TermColor *color);
void ecma48_repeat_byte(char ch, size_t count);

void put_control_code(StringView code);

void term_raw(void);
void term_cooked(void);

#endif
