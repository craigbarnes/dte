#ifndef SEARCH_H
#define SEARCH_H

#include <regex.h>
#include <stdbool.h>
#include "command/error.h"
#include "util/macros.h"
#include "view.h"

typedef enum {
    CSS_FALSE,
    CSS_TRUE,
    CSS_AUTO,
} SearchCaseSensitivity;

typedef struct {
    regex_t regex;
    char *pattern;
    int re_flags; // If zero, regex hasn't been compiled
    bool reverse;
} SearchState;

bool do_search_next(View *view, SearchState *search, ErrorBuffer *ebuf, SearchCaseSensitivity cs, bool skip) NONNULL_ARGS WARN_UNUSED_RESULT;

static inline void toggle_search_direction(SearchState *search)
{
    search->reverse ^= 1;
}

NONNULL_ARGS WARN_UNUSED_RESULT
static inline bool search_next(View *view, SearchState *search, ErrorBuffer *ebuf, SearchCaseSensitivity cs)
{
    return do_search_next(view, search, ebuf, cs, false);
}

NONNULL_ARGS WARN_UNUSED_RESULT
static inline bool search_prev(View *view, SearchState *search, ErrorBuffer *ebuf, SearchCaseSensitivity cs)
{
    toggle_search_direction(search);
    bool r = search_next(view, search, ebuf, cs);
    toggle_search_direction(search);
    return r;
}

bool search_tag(View *view, ErrorBuffer *ebuf, const char *pattern) NONNULL_ARGS WARN_UNUSED_RESULT;
void search_set_regexp(SearchState *search, const char *pattern) NONNULL_ARGS;
void search_free_regexp(SearchState *search) NONNULL_ARGS;

#endif
