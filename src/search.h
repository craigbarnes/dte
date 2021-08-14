#ifndef SEARCH_H
#define SEARCH_H

#include <regex.h>
#include <stdbool.h>
#include "util/macros.h"

typedef enum {
    SEARCH_FWD,
    SEARCH_BWD,
} SearchDirection;

typedef enum {
    REPLACE_CONFIRM = 1 << 0,
    REPLACE_GLOBAL = 1 << 1,
    REPLACE_IGNORE_CASE = 1 << 2,
    REPLACE_BASIC = 1 << 3,
    REPLACE_CANCEL = 1 << 4,
} ReplaceFlags;

typedef struct {
    regex_t regex;
    char *pattern;
    SearchDirection direction;
    int re_flags; // If zero, regex hasn't been compiled
} SearchState;

static inline void toggle_search_direction(SearchDirection *direction)
{
    *direction ^= 1;
}

bool search_tag(const char *pattern, bool *err);
void search_set_regexp(const char *pattern);
void search_prev(void);
void search_next(void);
void search_next_word(void);

void reg_replace(const char *pattern, const char *format, ReplaceFlags flags) NONNULL_ARGS;

#endif
