#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "feature.h"
#include "key.h"
#include "style.h"
#include "util/macros.h"

enum {
    TERM_INBUF_SIZE = 4096,
    TERM_OUTBUF_SIZE = 8192,
};

typedef enum {
    CURSOR_INVALID = -1,
    CURSOR_DEFAULT = 0,
    CURSOR_BLINKING_BLOCK = 1,
    CURSOR_STEADY_BLOCK = 2,
    CURSOR_BLINKING_UNDERLINE = 3,
    CURSOR_STEADY_UNDERLINE = 4,
    CURSOR_BLINKING_BAR = 5,
    CURSOR_STEADY_BAR = 6,
    CURSOR_KEEP = 7,
} TermCursorType;

typedef struct {
    TermCursorType type;
    int32_t color;
} TermCursorStyle;

typedef enum {
    TAB_NORMAL, // Render tabs as whitespace
    TAB_SPECIAL, // Render tabs according to `set display-special true` (">---")
    TAB_CONTROL, // Render tabs like other control characters ("^I")
} TermTabOutputMode;

typedef struct {
    // Current x position (tab: 1-8, double-width: 2, invalid UTF-8 byte: 4)
    // if smaller than scroll_x, printed characters are not visible
    size_t x;
    size_t scroll_x; // Number of characters scrolled (x direction)
    unsigned int count; // Number of buffered bytes (see term_output_flush())
    unsigned int width; // Width of terminal area being written to (see term_output_reset())
    uint8_t tab_mode; // See TermTabOutputMode
    uint8_t tab_width; // See LocalOptions::tab_width
    bool can_clear; // Whether lines can be cleared with EL (Erase in Line) sequence
    bool sync_pending; // See TFLAG_SYNC and term_end_sync_update()
    TermStyle style; // The style currently active in the terminal
    TermCursorStyle cursor_style; // The cursor style currently active in the terminal
    char buf[TERM_OUTBUF_SIZE]; // Buffer contents
} TermOutputBuffer;

typedef struct {
    unsigned int len;
    bool can_be_truncated;
    char buf[TERM_INBUF_SIZE];
} TermInputBuffer;

typedef struct {
    TermFeatureFlags features;
    unsigned int width; // Terminal width (in columns)
    unsigned int height; // Terminal height (in rows)
    unsigned int ncv_attributes; // See "no_color_video" terminfo(5) capability
    TermOutputBuffer obuf;
    TermInputBuffer ibuf;
} Terminal;

void term_init(Terminal *term, const char *name, const char *colorterm) NONNULL_ARG(1);
void term_enable_private_modes(Terminal *term) NONNULL_ARGS;
void term_restore_private_modes(Terminal *term) NONNULL_ARGS;
void term_restore_cursor_style(Terminal *term) NONNULL_ARGS;

#endif
