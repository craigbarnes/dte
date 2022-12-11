#ifndef REPLACE_H
#define REPLACE_H

#include <stdbool.h>
#include "util/macros.h"
#include "view.h"

typedef enum {
    REPLACE_CONFIRM = 1 << 0,
    REPLACE_GLOBAL = 1 << 1,
    REPLACE_IGNORE_CASE = 1 << 2,
    REPLACE_BASIC = 1 << 3,
    REPLACE_CANCEL = 1 << 4,
} ReplaceFlags;

bool reg_replace(View *view, const char *pattern, const char *format, ReplaceFlags flags) NONNULL_ARGS;

#endif
