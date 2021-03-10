#ifndef SEARCH_H
#define SEARCH_H

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

SearchDirection get_search_direction(void) PURE;
void set_search_direction(SearchDirection dir);
void toggle_search_direction(void);

bool search_tag(const char *pattern, bool *err);
void search_set_regexp(const char *pattern);
void search_prev(void);
void search_next(void);
void search_next_word(void);

void reg_replace(const char *pattern, const char *format, ReplaceFlags flags) NONNULL_ARGS;

#endif
