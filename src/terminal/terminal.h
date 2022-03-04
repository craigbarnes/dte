#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
#include <sys/types.h>
#include "color.h"
#include "key.h"
#include "output.h"
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
    TermColorCapabilityType color_type;
    TermFeatureFlags features;
    unsigned int width;
    unsigned int height;
    unsigned int ncv_attributes;
    TermControlCodes control_codes;
    ssize_t (*parse_key_sequence)(const char *buf, size_t length, KeyCode *key);
    void (*set_color)(TermOutputBuffer *obuf, const TermColor *color);
    void (*repeat_byte)(TermOutputBuffer *obuf, char ch, size_t count);
} Terminal;

extern Terminal terminal;

void term_init(const char *term) NONNULL_ARGS;
void term_enable_private_modes(const Terminal *term, TermOutputBuffer *obuf) NONNULL_ARGS;
void term_restore_private_modes(const Terminal *term, TermOutputBuffer *obuf) NONNULL_ARGS;

#endif
