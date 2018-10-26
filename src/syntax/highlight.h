#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include "../buffer.h"
#include "../terminal/color.h"

HlColor **hl_line(Buffer *b, const char *line, size_t len, int line_nr, int *next_changed);
void hl_fill_start_states(Buffer *b, int line_nr);
void hl_insert(Buffer *b, int first, int lines);
void hl_delete(Buffer *b, int first, int lines);

#endif
