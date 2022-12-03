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

void record_copy(Clipboard *clip, char *buf, size_t len, bool is_lines);
void copy(Clipboard *clip, View *view, size_t len, bool is_lines);
void cut(Clipboard *clip, View *view, size_t len, bool is_lines);
void paste(Clipboard *clip, View *view, bool at_cursor, bool move_after);

#endif
