#ifndef HL_H
#define HL_H

#include "buffer.h"

struct hl_color **hl_line(struct buffer *b, const char *line, int len, int line_nr, int *next_changed);
void hl_fill_start_states(struct buffer *b, int line_nr);
void hl_insert(struct buffer *b, int first, int lines);
void hl_delete(struct buffer *b, int first, int lines);

#endif
