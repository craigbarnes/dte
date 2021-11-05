#ifndef MOVE_H
#define MOVE_H

#include <stdbool.h>
#include <stddef.h>
#include "block-iter.h"
#include "view.h"

void move_to_preferred_x(View *view, long preferred_x);
void move_cursor_left(View *view);
void move_cursor_right(View *view);
void move_bol(View *view);
void move_bol_smart(View *view);
void move_eol(View *view);
void move_up(View *view, long count);
void move_down(View *view, long count);
void move_bof(View *view);
void move_eof(View *view);
void move_to_line(View *v, size_t line);
void move_to_column(View *v, size_t column);

size_t word_fwd(BlockIter *bi, bool skip_non_word);
size_t word_bwd(BlockIter *bi, bool skip_non_word);

#endif
