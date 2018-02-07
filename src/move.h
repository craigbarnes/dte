#ifndef MOVE_H
#define MOVE_H

#include <stdbool.h>
#include "block-iter.h"
#include "view.h"

void move_to_preferred_x(int preferred_x);
void move_cursor_left(void);
void move_cursor_right(void);
void move_bol(void);
void move_eol(void);
void move_up(int count);
void move_down(int count);
void move_bof(void);
void move_eof(void);
void move_to_line(View *v, int line);
void move_to_column(View *v, int column);

long word_fwd(BlockIter *bi, bool skip_non_word);
long word_bwd(BlockIter *bi, bool skip_non_word);

#endif
