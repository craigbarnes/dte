#ifndef SYNTAX_HIGHLIGHT_H
#define SYNTAX_HIGHLIGHT_H

#include <stdbool.h>
#include <stddef.h>
#include "block-iter.h"
#include "syntax/color.h"
#include "syntax/syntax.h"
#include "util/ptr-array.h"
#include "util/string-view.h"

const TermColor **hl_line (
    Syntax *syn,
    PointerArray *line_start_states,
    const ColorScheme *cs,
    const StringView *line,
    size_t line_nr,
    bool *next_changed
);

void hl_fill_start_states (
    Syntax *syn,
    PointerArray *line_start_states,
    const ColorScheme *cs,
    BlockIter *bi,
    size_t line_nr
);

void hl_insert(PointerArray *line_start_states, size_t first, size_t lines);
void hl_delete(PointerArray *line_start_states, size_t first, size_t lines);

#endif
