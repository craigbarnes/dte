#ifndef SYNTAX_HIGHLIGHT_H
#define SYNTAX_HIGHLIGHT_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "syntax/color.h"
#include "util/string-view.h"

const TermColor **hl_line(Buffer *buffer, const ColorScheme *cs, const StringView *line, size_t line_nr, bool *next_changed);
void hl_fill_start_states(Buffer *buffer, const ColorScheme *cs, size_t line_nr);
void hl_insert(Buffer *buffer, size_t first, size_t lines);
void hl_delete(Buffer *buffer, size_t first, size_t lines);

#endif
