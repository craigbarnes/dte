#ifndef TERMINAL_ECMA48_H
#define TERMINAL_ECMA48_H

#include <stddef.h>
#include "color.h"
#include "output.h"

void ecma48_clear_screen(TermOutputBuffer *obuf);
void ecma48_clear_to_eol(TermOutputBuffer *obuf);
void ecma48_move_cursor(TermOutputBuffer *obuf, unsigned int x, unsigned int y);
void ecma48_set_color(TermOutputBuffer *obuf, const TermColor *color);
void ecma48_repeat_byte(TermOutputBuffer *obuf, char ch, size_t count);

#endif
