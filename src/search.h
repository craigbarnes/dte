#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>
#include "editor.h"
#include "util/macros.h"
#include "view.h"

typedef enum {
    REPLACE_CONFIRM = 1 << 0,
    REPLACE_GLOBAL = 1 << 1,
    REPLACE_IGNORE_CASE = 1 << 2,
    REPLACE_BASIC = 1 << 3,
    REPLACE_CANCEL = 1 << 4,
} ReplaceFlags;

static inline void toggle_search_direction(SearchDirection *direction)
{
    *direction ^= 1;
}

bool search_tag(View *view, const char *pattern, bool *err);
void search_set_regexp(SearchState *search, const char *pattern);
void search_prev(EditorState *e);
void search_next(EditorState *e);
void search_next_word(EditorState *e);

void reg_replace(EditorState *e, const char *pattern, const char *format, ReplaceFlags flags) NONNULL_ARGS;

#endif
