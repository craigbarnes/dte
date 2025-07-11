#ifndef SYNTAX_HIGHLIGHT_H
#define SYNTAX_HIGHLIGHT_H

#include <stdbool.h>
#include <stddef.h>
#include "block-iter.h"
#include "syntax/color.h"
#include "syntax/syntax.h"
#include "terminal/style.h"
#include "util/debug.h"
#include "util/ptr-array.h"
#include "util/string-view.h"

// Set styles in range [start,end] and return number of styles set
static inline size_t set_style_range (
    const TermStyle **styles,
    const TermStyle *emit_style,
    size_t start,
    size_t end
) {
    BUG_ON(start > end);
    for (size_t i = start; i < end; i++) {
        styles[i] = emit_style;
    }
    return end - start;
}

const TermStyle **hl_line (
    Syntax *syn,
    PointerArray *line_start_states,
    const StyleMap *sm,
    const StringView *line,
    size_t line_nr,
    bool *next_changed
);

void hl_fill_start_states (
    Syntax *syn,
    PointerArray *line_start_states,
    const StyleMap *sm,
    BlockIter *bi,
    size_t line_nr
);

void hl_insert(PointerArray *line_start_states, size_t first, size_t lines);
void hl_delete(PointerArray *line_start_states, size_t first, size_t lines);

#endif
