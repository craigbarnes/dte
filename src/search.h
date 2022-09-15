#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>
#include "editor.h"
#include "util/macros.h"
#include "view.h"

static inline void toggle_search_direction(SearchDirection *direction)
{
    *direction ^= 1;
}

bool search_tag(View *view, const char *pattern, bool *err);
void search_set_regexp(SearchState *search, const char *pattern);
void search_free_regexp(SearchState *search);
void search_prev(EditorState *e);
void search_next(EditorState *e);
void search_next_word(EditorState *e);

#endif
