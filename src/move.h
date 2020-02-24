#ifndef MOVE_H
#define MOVE_H

#include <stdbool.h>
#include <stddef.h>
#include "block-iter.h"
#include "view.h"

void move_to_preferred_x(long preferred_x);
void move_cursor_left(void);
void move_cursor_right(void);
void move_bol(void);
void move_bol_smart(void);
void move_eol(void);
void move_up(long count);
void move_down(long count);
void move_bof(void);
void move_eof(void);
void move_to_line(View *v, size_t line);
void move_to_column(View *v, size_t column);

size_t word_fwd(BlockIter *bi, bool skip_non_word);
size_t word_bwd(BlockIter *bi, bool skip_non_word);

#endif
