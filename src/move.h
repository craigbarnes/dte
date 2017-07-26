#ifndef MOVE_H
#define MOVE_H

#include "libc.h"

struct block_iter;
struct view;

void move_to_preferred_x(int preferred_x);
void move_cursor_left(void);
void move_cursor_right(void);
void move_bol(void);
void move_eol(void);
void move_up(int count);
void move_down(int count);
void move_bof(void);
void move_eof(void);
void move_to_line(struct view *v, int line);
void move_to_column(struct view *v, int column);

long word_fwd(struct block_iter *bi, bool skip_non_word);
long word_bwd(struct block_iter *bi, bool skip_non_word);

#endif
