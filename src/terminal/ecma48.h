#ifndef TERMINAL_ECMA48_H
#define TERMINAL_ECMA48_H

#include "color.h"

void ecma48_clear_to_eol(void);
void ecma48_move_cursor(int x, int y);
void ecma48_set_color(const TermColor *color);

void term_raw(void);
void term_cooked(void);

#endif
