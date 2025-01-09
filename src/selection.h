#ifndef SELECTION_H
#define SELECTION_H

#include <stdbool.h>
#include <stddef.h>
#include "block-iter.h"
#include "buffer.h"
#include "util/macros.h"
#include "util/string-view.h"
#include "view.h"

typedef struct {
    BlockIter si;
    size_t so;
    size_t eo;
    bool swapped;
} SelectionInfo;

static inline void unselect(View *view)
{
    view->select_mode = SELECT_NONE;
    if (view->selection) {
        view->selection = SELECT_NONE;
        mark_all_lines_changed(view->buffer);
    }
}

SelectionInfo init_selection(const View *view) NONNULL_ARGS WARN_UNUSED_RESULT;
size_t prepare_selection(View *view) NONNULL_ARGS WARN_UNUSED_RESULT;
size_t get_nr_selected_lines(const SelectionInfo *info) NONNULL_ARGS WARN_UNUSED_RESULT;
size_t get_nr_selected_chars(const SelectionInfo *info) NONNULL_ARGS WARN_UNUSED_RESULT;

void select_block(View *view) NONNULL_ARGS;
bool line_has_opening_brace(StringView line) WARN_UNUSED_RESULT;
bool line_has_closing_brace(StringView line) WARN_UNUSED_RESULT;

#endif
