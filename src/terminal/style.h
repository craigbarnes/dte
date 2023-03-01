#ifndef TERMINAL_STYLE_H
#define TERMINAL_STYLE_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "color.h"
#include "util/macros.h"
#include "util/ptr-array.h"

enum {
    ATTR_KEEP = 0x01,
    ATTR_UNDERLINE = 0x02,
    ATTR_REVERSE = 0x04,
    ATTR_BLINK = 0x08,
    ATTR_DIM = 0x10,
    ATTR_BOLD = 0x20,
    ATTR_INVIS = 0x40,
    ATTR_ITALIC = 0x80,
    ATTR_STRIKETHROUGH = 0x100,
};

typedef struct {
    int32_t fg;
    int32_t bg;
    unsigned int attr;
} TermColor;

static inline bool same_color(const TermColor *a, const TermColor *b)
{
    return a->fg == b->fg && a->bg == b->bg && a->attr == b->attr;
}

ssize_t parse_term_color(TermColor *color, char **strs, size_t nstrs) NONNULL_ARGS WARN_UNUSED_RESULT;
size_t color_to_str(char *buf, int32_t color) NONNULL_ARGS WARN_UNUSED_RESULT;
const char *term_color_to_string(const TermColor *color) NONNULL_ARGS_AND_RETURN;
void collect_colors_and_attributes(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
