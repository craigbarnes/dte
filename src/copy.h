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

void copy(Clipboard *clip, View *v, size_t len, bool is_lines);
void cut(Clipboard *clip, View *v, size_t len, bool is_lines);
void paste(Clipboard *clip, View *v, bool at_cursor);

#endif
