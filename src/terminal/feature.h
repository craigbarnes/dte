#ifndef TERMINAL_FEATURE_H
#define TERMINAL_FEATURE_H

#include "util/macros.h"

// Bit flags representing supported terminal features
// See also: KEYCODE_QUERY_REPLY_BIT
typedef enum {
    TFLAG_BACK_COLOR_ERASE = 1 << 0, // Can erase line/area with non-default background color
    TFLAG_ECMA48_REPEAT = 1 << 1, // Supports ECMA-48 "REP" (repeat character; §8.3.103)
    TFLAG_SET_WINDOW_TITLE = 1 << 2, // Supports xterm sequences for setting window title
    TFLAG_RXVT = 1 << 3, // Emits rxvt-specific sequences for some key combos (see rxvt.c)
    TFLAG_LINUX = 1 << 4, // Emits linux-specific sequences for F1-F5 (see linux.c)
    TFLAG_OSC52_COPY = 1 << 5, // Supports OSC 52 clipboard copy operations
    TFLAG_META_ESC = 1 << 6, // Enable xterm "metaSendsEscape" mode (via DECSET 1036)
    TFLAG_ALT_ESC = 1 << 7, // Enable xterm "altSendsEscape" mode (via DECSET 1039)
    TFLAG_KITTY_KEYBOARD = 1 << 8, // Supports kitty keyboard protocol
    TFLAG_SYNC = 1 << 9, // Supports synchronized updates (via DECSET 2026)
    TFLAG_QUERY_L2 = 1 << 10, // Supports or tolerates queries sent by term_put_level_2_queries()
    TFLAG_QUERY_L3 = 1 << 11, // Supports or tolerates queries sent by term_put_level_3_queries()
    TFLAG_NO_QUERY_L1 = 1 << 12, // Don't emit *any* queries
    TFLAG_NO_QUERY_L3 = 1 << 13, // Don't emit L3 queries (special case override of TFLAG_QUERY_L3)
    TFLAG_8_COLOR = 1 << 14, // Supports ECMA-48 palette colors (e.g. SGR 30)
    TFLAG_16_COLOR = 1 << 15, // Supports aixterm-style "bright" palette colors (e.g. SGR 90)
    TFLAG_256_COLOR = 1 << 16, // Supports xterm-style (ISO 8613-6) indexed colors (e.g. SGR 38;5;255)
    TFLAG_TRUE_COLOR = 1 << 17, // Supports 24-bit RGB ("direct") colors (e.g. SGR 38;2;50;60;70)
    TFLAG_MODIFY_OTHER_KEYS = 1 << 18, // Supports xterm "modifyOtherKeys" mode (keyboard protocol)
    TFLAG_DEL_CTRL_BACKSPACE = 1 << 19, // Emits DEL character (exclusively) for Ctrl+Backspace
    TFLAG_BS_CTRL_BACKSPACE = 1 << 20, // Emits BS character for Ctrl+Backspace (and Ctrl+h, but not Backspace)
    TFLAG_NCV_UNDERLINE = 1 << 21, // Colors can't be used with ATTR_UNDERLINE (see "ncv" in terminfo(5))
    TFLAG_NCV_DIM = 1 << 22, // Colors can't be used with ATTR_DIM
    TFLAG_NCV_REVERSE = 1 << 23, // Colors can't be used with ATTR_REVERSE
} TermFeatureFlags;

TermFeatureFlags term_get_features(const char *name, const char *colorterm);
const char *term_feature_to_str(TermFeatureFlags flag) RETURNS_NONNULL;

#endif
