#ifndef TERMINAL_ECMA48_H
#define TERMINAL_ECMA48_H

#include <stddef.h>
#include "color.h"

void ecma48_clear_screen(void);
void ecma48_clear_to_eol(void);
void ecma48_move_cursor(unsigned int x, unsigned int y);
void ecma48_set_color(const TermColor *color);
void ecma48_repeat_byte(char ch, size_t count);

#endif
