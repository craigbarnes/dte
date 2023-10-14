#ifndef TERMINAL_CURSOR_H
#define TERMINAL_CURSOR_H

#include <stdbool.h>
#include <stdint.h>
#include "terminal/color.h"
#include "terminal/terminal.h"
#include "util/macros.h"
#include "util/ptr-array.h"

typedef enum {
    CURSOR_MODE_DEFAULT,
    CURSOR_MODE_INSERT,
    CURSOR_MODE_OVERWRITE,
    CURSOR_MODE_CMDLINE,
    NR_CURSOR_MODES,
} CursorInputMode;

static inline bool cursor_color_is_valid(int32_t c)
{
    return c == COLOR_KEEP || c == COLOR_DEFAULT || color_is_rgb(c);
}

static inline bool cursor_type_is_valid(TermCursorType type)
{
    return type >= CURSOR_DEFAULT && type <= CURSOR_KEEP;
}

static inline bool same_cursor(const TermCursorStyle *a, const TermCursorStyle *b)
{
    return a->type == b->type && a->color == b->color;
}

static inline TermCursorStyle get_default_cursor_style(CursorInputMode mode)
{
    bool is_default_mode = (mode == CURSOR_MODE_DEFAULT);
    return (TermCursorStyle) {
        .type = is_default_mode ? CURSOR_DEFAULT : CURSOR_KEEP,
        .color = is_default_mode ? COLOR_DEFAULT : COLOR_KEEP,
    };
}

const char *cursor_mode_to_str(CursorInputMode mode) RETURNS_NONNULL;
const char *cursor_type_to_str(TermCursorType type) RETURNS_NONNULL;
const char *cursor_color_to_str(int32_t color) RETURNS_NONNULL;
CursorInputMode cursor_mode_from_str(const char *name) NONNULL_ARGS;
TermCursorType cursor_type_from_str(const char *name) NONNULL_ARGS;
int32_t cursor_color_from_str(const char *str) NONNULL_ARGS;
void collect_cursor_modes(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_cursor_types(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_cursor_colors(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
