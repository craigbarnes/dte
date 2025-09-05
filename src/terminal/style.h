#ifndef TERMINAL_STYLE_H
#define TERMINAL_STYLE_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "color.h"
#include "util/macros.h"
#include "util/ptr-array.h"

#define COLOR_STR_BUFSIZE 16
#define TERM_STYLE_BUFSIZE 128

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
} TermStyle;

static inline TermStyle term_style(int32_t fg, int32_t bg, unsigned int attr)
{
    return (TermStyle){.fg = fg, .bg = bg, .attr = attr};
}

static inline bool same_style(const TermStyle *a, const TermStyle *b)
{
    return a->fg == b->fg && a->bg == b->bg && a->attr == b->attr;
}

static inline void mask_style2(TermStyle *style, const TermStyle *over, bool mask_bg)
{
    *style = (TermStyle) {
        .fg = (over->fg == COLOR_KEEP) ? style->fg : over->fg,
        .bg = (over->bg == COLOR_KEEP || !mask_bg) ? style->bg : over->bg,
        .attr = (over->attr & ATTR_KEEP) ? style->attr : over->attr,
    };
}

static inline void mask_style(TermStyle *style, const TermStyle *over)
{
    return mask_style2(style, over, true);
}

ssize_t parse_term_style(TermStyle *style, char **strs, size_t nstrs) NONNULL_ARGS WARN_UNUSED_RESULT;
size_t color_to_str(char buf[static COLOR_STR_BUFSIZE], int32_t color) NONNULL_ARGS WARN_UNUSED_RESULT;
const char *term_style_to_string(char buf[static TERM_STYLE_BUFSIZE], const TermStyle *style) NONNULL_ARGS_AND_RETURN;
void collect_colors_and_attributes(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
