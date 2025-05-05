#ifndef INDENT_H
#define INDENT_H

#include <stdbool.h>
#include <stddef.h>
#include "block-iter.h"
#include "options.h"
#include "util/bit.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/string-view.h"

typedef struct {
    size_t bytes; // Size in bytes
    size_t width; // Width in columns
    size_t level; // Number of whole `indent-width` levels
    bool wsonly; // Empty or whitespace-only line

    // Only spaces or tabs, depending on `use_spaces_for_indent()`.
    // Note that a "sane" line can contain spaces after tabs for alignment.
    bool sane;
} IndentInfo;

// Divide `x` by `d`, to obtain the number of whole indent levels.
// If `d` is a power of 2, shift right by `u32_ctz(d)` instead, to
// avoid the expensive divide operation. This optimization applies
// to widths of 1, 2, 4 and 8, which covers all of the sensible ones.
static inline size_t indent_level(size_t x, size_t d)
{
    BUG_ON(d - 1 >= INDENT_WIDTH_MAX);
    return likely(IS_POWER_OF_2(d)) ? x >> u32_ctz(d) : x / d;
}

static inline size_t indent_remainder(size_t x, size_t m)
{
    BUG_ON(m - 1 >= INDENT_WIDTH_MAX);
    return likely(IS_POWER_OF_2(m)) ? x & (m - 1) : x % m;
}

static inline size_t next_indent_width(size_t x, size_t m)
{
    BUG_ON(m - 1 >= INDENT_WIDTH_MAX);
    return likely(IS_POWER_OF_2(m)) ? next_multiple(x + 1, m) : ((x + m) / m) * m;
}

char *make_indent(const LocalOptions *options, size_t width);
char *get_indent_for_next_line(const LocalOptions *options, const StringView *line);
IndentInfo get_indent_info(const LocalOptions *options, const StringView *line);
size_t get_indent_width(const StringView *line, unsigned int tab_width);
size_t get_indent_level_bytes_left(const LocalOptions *options, const BlockIter *cursor);
size_t get_indent_level_bytes_right(const LocalOptions *options, const BlockIter *cursor);

#endif
