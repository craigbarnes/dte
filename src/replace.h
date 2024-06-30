#ifndef REPLACE_H
#define REPLACE_H

#include <stdbool.h>
#include "util/macros.h"
#include "view.h"

typedef enum {
    REPLACE_CONFIRM = 1 << 0, // Show confirmation prompt for each replacement
    REPLACE_GLOBAL = 1 << 1, // Allow pattern to match multiple times per line
    REPLACE_IGNORE_CASE = 1 << 2, // Enable case-insensitive matching
    REPLACE_BASIC = 1 << 3, // Use basic (POSIX BRE) regex syntax
    REPLACE_CANCEL = 1 << 4, // Used internally by reg_replace()
} ReplaceFlags;

bool reg_replace(View *view, const char *pattern, const char *format, ReplaceFlags flags) NONNULL_ARGS;

#endif
