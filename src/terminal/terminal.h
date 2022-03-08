#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
#include <sys/types.h>
#include "color.h"
#include "key.h"
#include "util/macros.h"
#include "util/string-view.h"

typedef enum {
    TFLAG_BACK_COLOR_ERASE = 0x01, // Can erase with specific background color
    TFLAG_ECMA48_REPEAT = 0x02, // Supports ECMA-48 "REP" (repeat character; §8.3.103)
    TFLAG_SET_WINDOW_TITLE = 0x04, // Supports xterm sequences for setting window title
    TFLAG_RXVT = 0x08, // Emits rxvt-specific sequences for some key combos (see rxvt.c)
    TFLAG_LINUX = 0x10, // Emits linux-specific sequences for F1-F5 (see linux.c)
    TFLAG_OSC52_COPY = 0x20, // Supports OSC 52 clipboard copy operations
    TFLAG_META_ESC = 0x40, // Try to enable {meta,alt}SendsEscape modes at startup
    TFLAG_KITTY_KEYBOARD = 0x80, // Supports kitty keyboard protocol (at least mode 0b1)
    TFLAG_ITERM2 = 0x100, // Supports extended keyboard protocol via "\e[>1u" (but not "\e[>5u")
} TermFeatureFlags;

typedef struct {
    StringView keypad_off;
    StringView keypad_on;
    StringView cup_mode_off;
    StringView cup_mode_on;
    StringView show_cursor;
    StringView hide_cursor;
    StringView save_title;
    StringView restore_title;
    StringView set_title_begin;
    StringView set_title_end;
} TermControlCodes;

typedef struct {
    char *buf;
    size_t count;
    size_t scroll_x; // Number of characters scrolled (x direction)

    // Current x position (tab: 1-8, double-width: 2, invalid UTF-8 byte: 4)
    // if smaller than scroll_x printed characters are not visible
    size_t x;

    size_t width;
    enum {
        TAB_NORMAL,
        TAB_SPECIAL,
        TAB_CONTROL,
    } tab;
    uint8_t tab_width;
    bool can_clear;
    TermColor color;
} TermOutputBuffer;

typedef struct {
    char buf[256];
    size_t len;
    bool can_be_truncated;
} TermInputBuffer;

typedef struct Terminal {
    TermColorCapabilityType color_type;
    TermFeatureFlags features;
    unsigned int width;
    unsigned int height;
    unsigned int ncv_attributes;
    TermControlCodes control_codes;
    ssize_t (*parse_key_sequence)(const char *buf, size_t length, KeyCode *key);
    TermOutputBuffer obuf;
    TermInputBuffer ibuf;
} Terminal;

void term_init(Terminal *term, const char *name) NONNULL_ARGS;
void term_enable_private_modes(const Terminal *term, TermOutputBuffer *obuf) NONNULL_ARGS;
void term_restore_private_modes(const Terminal *term, TermOutputBuffer *obuf) NONNULL_ARGS;

#endif
