#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "key.h"
#include "style.h"
#include "util/macros.h"

enum {
    TERM_INBUF_SIZE = 4096,
    TERM_OUTBUF_SIZE = 8192,
};

// Bit flags representing supported terminal features
// See also: KEYCODE_QUERY_REPLY_BIT
typedef enum {
    TFLAG_BACK_COLOR_ERASE = 0x01, // Can erase line/area with non-default background color
    TFLAG_ECMA48_REPEAT = 0x02, // Supports ECMA-48 "REP" (repeat character; §8.3.103)
    TFLAG_SET_WINDOW_TITLE = 0x04, // Supports xterm sequences for setting window title
    TFLAG_RXVT = 0x08, // Emits rxvt-specific sequences for some key combos (see rxvt.c)
    TFLAG_LINUX = 0x10, // Emits linux-specific sequences for F1-F5 (see linux.c)
    TFLAG_OSC52_COPY = 0x20, // Supports OSC 52 clipboard copy operations
    TFLAG_META_ESC = 0x40, // Enable xterm "metaSendsEscape" mode (via DECSET 1036)
    TFLAG_ALT_ESC = 0x80, // Enable xterm "altSendsEscape" mode (via DECSET 1039)
    TFLAG_KITTY_KEYBOARD = 0x100, // Supports kitty keyboard protocol
    TFLAG_SYNC = 0x400, // Supports synchronized updates (via DECSET 2026)
    TFLAG_QUERY_L2 = 0x800, // Supports or tolerates queries sent by term_put_level_2_queries()
    TFLAG_QUERY_L3 = 0x1000, // Supports or tolerates queries sent by term_put_level_3_queries()
    TFLAG_NO_QUERY_L3 = 0x2000, // Don't emit L3 queries (special case override of TFLAG_QUERY_L3)
    TFLAG_8_COLOR = 0x4000, // Supports ECMA-48 palette colors (e.g. SGR 30)
    TFLAG_16_COLOR = 0x8000, // Supports aixterm-style "bright" palette colors (e.g. SGR 90)
    TFLAG_256_COLOR = 0x10000, // Supports xterm-style (ISO 8613-6) indexed colors (e.g. SGR 38;5;255)
    TFLAG_TRUE_COLOR = 0x20000, // Supports 24-bit RGB ("direct") colors (e.g. SGR 38;2;50;60;70)
    TFLAG_MODIFY_OTHER_KEYS = 0x40000, // Supports xterm "modifyOtherKeys" mode (keyboard protocol)
    TFLAG_DEL_CTRL_BACKSPACE = 0x80000, // Emits DEL character (exclusively) for Ctrl+Backspace
    TFLAG_BS_CTRL_BACKSPACE = 0x100000, // Emits BS character for Ctrl+Backspace (and Ctrl+h, but not Backspace)
    TFLAG_NCV_UNDERLINE = 0x200000, // Colors can't be used with ATTR_UNDERLINE (see "ncv" in terminfo(5))
    TFLAG_NCV_DIM = 0x400000, // Colors can't be used with ATTR_DIM
    TFLAG_NCV_REVERSE = 0x800000, // Colors can't be used with ATTR_REVERSE
} TermFeatureFlags;

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
    ssize_t (*parse_input)(const char *buf, size_t length, KeyCode *key);
    TermOutputBuffer obuf;
    TermInputBuffer ibuf;
} Terminal;

void term_init(Terminal *term, const char *name, const char *colorterm) NONNULL_ARG(1);
void term_enable_private_modes(Terminal *term) NONNULL_ARGS;
void term_restore_private_modes(Terminal *term) NONNULL_ARGS;
void term_restore_cursor_style(Terminal *term) NONNULL_ARGS;

#endif
