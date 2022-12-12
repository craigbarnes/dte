#ifndef COPY_H
#define COPY_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"
#include "view.h"

typedef struct {
    char *buf;
    size_t len;
    bool is_lines;
} Clipboard;

typedef enum {
    PASTE_LINES_BELOW_CURSOR,
    PASTE_LINES_ABOVE_CURSOR,
    PASTE_LINES_INLINE,
} PasteLinesType;

void record_copy(Clipboard *clip, char *buf, size_t len, bool is_lines);
void copy(Clipboard *clip, View *view, size_t len, bool is_lines);
void cut(Clipboard *clip, View *view, size_t len, bool is_lines);
void paste(Clipboard *clip, View *view, PasteLinesType type, bool move_after);

#endif
