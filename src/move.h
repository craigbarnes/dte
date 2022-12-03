#ifndef MOVE_H
#define MOVE_H

#include <stdbool.h>
#include <stddef.h>
#include "block-iter.h"
#include "view.h"

typedef enum {
    BOL_SMART = 0x1, // Move to end of indent, before moving to bol (left moves only)
    BOL_SMART_TOGGLE = 0x2, // Move to end of indent, if at bol (can move right)
} SmartBolFlags;

void move_to_preferred_x(View *view, long preferred_x);
void move_cursor_left(View *view);
void move_cursor_right(View *view);
void move_bol(View *view);
void move_bol_smart(View *view, SmartBolFlags flags);
void move_eol(View *view);
void move_up(View *view, long count);
void move_down(View *view, long count);
void move_bof(View *view);
void move_eof(View *view);
void move_to_line(View *view, size_t line);
void move_to_column(View *view, size_t column);
void move_to_filepos(View *view, size_t line, size_t column);

size_t word_fwd(BlockIter *bi, bool skip_non_word);
size_t word_bwd(BlockIter *bi, bool skip_non_word);

#endif
