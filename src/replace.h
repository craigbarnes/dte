#ifndef REPLACE_H
#define REPLACE_H

#include "editor.h"
#include "util/macros.h"

typedef enum {
    REPLACE_CONFIRM = 1 << 0,
    REPLACE_GLOBAL = 1 << 1,
    REPLACE_IGNORE_CASE = 1 << 2,
    REPLACE_BASIC = 1 << 3,
    REPLACE_CANCEL = 1 << 4,
} ReplaceFlags;

void reg_replace(EditorState *e, const char *pattern, const char *format, ReplaceFlags flags) NONNULL_ARGS;

#endif
