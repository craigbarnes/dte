#ifndef UTIL_LINE_ITER_H
#define UTIL_LINE_ITER_H

#include <stddef.h>
#include <string.h>
#include "debug.h"
#include "macros.h"
#include "string-view.h"

static inline char *buf_next_line(char *buf, size_t *posp, size_t size)
{
    size_t pos = *posp;
    BUG_ON(pos >= size);
    size_t avail = size - pos;
    char *line = buf + pos;
    char *nl = memchr(line, '\n', avail);
    if (nl) {
        *nl = '\0';
        *posp += nl - line + 1;
    } else {
        line[avail] = '\0';
        *posp += avail;
    }
    return line;
}

static inline StringView buf_slice_next_line(const char *buf, size_t *posp, size_t size)
{
    size_t pos = *posp;
    BUG_ON(pos >= size);
    size_t avail = size - pos;
    const char *line = buf + pos;
    const char *nl = memchr(line, '\n', avail);
    size_t line_length = nl ? (nl - line + 1) : avail;
    *posp += line_length;
    return string_view(line, line_length);
}

#endif
