#ifndef TERMINAL_KEY_H
#define TERMINAL_KEY_H

#include <stddef.h>
#include <stdint.h>
#include "util/ascii.h"
#include "util/macros.h"

enum {
    // Maximum length of string produced by keycode_to_string()
    KEYCODE_STR_MAX = 32 // sizeof("QUERY REPLY; 0x12345678")
};

enum {
    KEY_NONE = 0,
    KEY_TAB = '\t',
    KEY_ENTER = '\n',
    KEY_SPACE = ' ',

    // This is the maximum Unicode codepoint allowed by RFC 3629.
    // When stored in a 32-bit integer, it only requires the first
    // 21 low-order bits, leaving 11 high-order bits available to
    // be used as bit flags.
    KEY_UNICODE_MAX = 0x10FFFF,

    // In addition to the 11 unused, high-order bits, there are also
    // 983,042 unused values in the range from KEY_UNICODE_MAX + 1 to
    // (1 << 21) - 1, which can be used to represent special keys.
    KEY_SPECIAL_MIN = KEY_UNICODE_MAX + 1,

    // Note: these must be kept in sync with the array of names in key.c
    KEY_INSERT = KEY_SPECIAL_MIN,
    KEY_DELETE,
    KEY_UP,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_BEGIN,
    KEY_END,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_PAGE_UP,
    KEY_SCROLL_LOCK,
    KEY_PRINT_SCREEN,
    KEY_PAUSE,
    KEY_MENU,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,

    KEY_SPECIAL_MAX = KEY_F20,
    NR_SPECIAL_KEYS = KEY_SPECIAL_MAX - KEY_SPECIAL_MIN + 1,

    // In-band signalling for non-key events
    KEYCODE_DETECTED_PASTE = KEY_SPECIAL_MAX + 1,
    KEYCODE_BRACKETED_PASTE,
    KEY_IGNORE,

    // Modifier bit flags (as described above)
    KEYCODE_MODIFIER_OFFSET = 21,
    MOD_SHIFT =  1 << KEYCODE_MODIFIER_OFFSET,
    MOD_META  =  2 << KEYCODE_MODIFIER_OFFSET,
    MOD_CTRL  =  4 << KEYCODE_MODIFIER_OFFSET,
    MOD_SUPER =  8 << KEYCODE_MODIFIER_OFFSET,
    MOD_HYPER = 16 << KEYCODE_MODIFIER_OFFSET,
    MOD_MASK  = MOD_SHIFT | MOD_META | MOD_CTRL | MOD_SUPER | MOD_HYPER,

    // If this bit is set, all other bits correspond to TermFeatureFlags.
    // This is used by the functions in query.c to communicate replies to
    // term_read_input(), although such values are handled entirely within
    // that function and are never present in KeyCode values it returns.
    KEYCODE_QUERY_REPLY_BIT = 1u << 30,
};

typedef uint32_t KeyCode;

static inline KeyCode keycode_get_key(KeyCode k)
{
    return k & ~MOD_MASK;
}

static inline KeyCode keycode_get_modifiers(KeyCode k)
{
    return k & MOD_MASK;
}

static inline KeyCode keycode_normalize(KeyCode k)
{
    switch (k) {
    case '\t': return KEY_TAB;
    case '\r': return KEY_ENTER;
    case 0x7F: return MOD_CTRL | '?';
    }
    if (k < 0x20) {
        return MOD_CTRL | ascii_tolower(k | 0x40);
    }
    return k;
}

KeyCode parse_key_string(const char *str) NONNULL_ARGS WARN_UNUSED_RESULT;
size_t keycode_to_string(KeyCode key, char *buf) NONNULL_ARGS;

#endif
