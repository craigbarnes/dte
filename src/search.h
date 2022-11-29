#ifndef SEARCH_H
#define SEARCH_H

#include <regex.h>
#include <stdbool.h>
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

static inline void toggle_search_direction(SearchState *search)
{
    search->reverse ^= 1;
}

bool search_tag(View *view, const char *pattern);
void search_set_regexp(SearchState *search, const char *pattern);
void search_free_regexp(SearchState *search);
void search_prev(View *view, SearchState *search, SearchCaseSensitivity cs);
void search_next(View *view, SearchState *search, SearchCaseSensitivity cs);
void search_next_word(View *view, SearchState *search, SearchCaseSensitivity cs);

#endif
