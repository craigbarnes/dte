#ifndef COPY_H
#define COPY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "util/debug.h"
#include "util/macros.h"
#include "view.h"

typedef struct {
    char NONSTRING *buf;
    size_t len;
    bool is_lines;
} Clipboard;

typedef enum {
    PASTE_LINES_BELOW_CURSOR,
    PASTE_LINES_ABOVE_CURSOR,
    PASTE_LINES_INLINE,
} PasteLinesType;

static inline void record_copy(Clipboard *clip, char *buf, size_t len, bool is_lines)
{
    BUG_ON(len && !buf);
    free(clip->buf);
    clip->buf = buf; // Takes ownership
    clip->len = len;
    clip->is_lines = is_lines;
}

void paste(Clipboard *clip, View *view, PasteLinesType type, bool move_after) NONNULL_ARGS;

#endif
