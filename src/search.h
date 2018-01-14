#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>
#include "macros.h"

typedef enum {
    SEARCH_FWD,
    SEARCH_BWD,
} SearchDirection;

typedef enum {
    REPLACE_CONFIRM = (1 << 0),
    REPLACE_GLOBAL = (1 << 1),
    REPLACE_IGNORE_CASE = (1 << 2),
    REPLACE_BASIC = (1 << 3),
    REPLACE_CANCEL = (1 << 4),
} ReplaceFlags;

bool search_tag(const char *pattern, bool *err);

void search_set_direction(SearchDirection dir);
SearchDirection current_search_direction(void);
void search_set_regexp(const char *pattern);
void search_prev(void);
void search_next(void);
void search_next_word(void);

NONNULL_ARGS
void reg_replace(const char *pattern, const char *format, ReplaceFlags flags);

#endif
