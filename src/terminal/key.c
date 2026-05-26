#include <string.h>
#include "key.h"
#include "util/array.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/str-util.h"
#include "util/unicode.h"
#include "util/utf8.h"

#define K(key) [key - KEY_SPECIAL_MIN]

static const char special_names[][10] = {
    K(KEY_ESCAPE) = "escape",
    K(KEY_BACKSPACE) = "backspace",
    K(KEY_INSERT) = "insert",
    K(KEY_DELETE) = "delete",
    K(KEY_HOME) = "home",
    K(KEY_END) = "end",
    K(KEY_PAGE_UP) = "pgup",
    K(KEY_PAGE_DOWN) = "pgdown",
    K(KEY_UP) = "up",
    K(KEY_DOWN) = "down",
    K(KEY_RIGHT) = "right",
    K(KEY_LEFT) = "left",
    K(KEY_BEGIN) = "begin",
    K(KEY_SCROLL_LOCK) = "scrlock",
    K(KEY_PRINT_SCREEN) = "print",
    K(KEY_PAUSE) = "pause",
    K(KEY_MENU) = "menu",
    K(KEY_F1) = "F1",
    K(KEY_F2) = "F2",
    K(KEY_F3) = "F3",
    K(KEY_F4) = "F4",
    K(KEY_F5) = "F5",
    K(KEY_F6) = "F6",
    K(KEY_F7) = "F7",
    K(KEY_F8) = "F8",
    K(KEY_F9) = "F9",
    K(KEY_F10) = "F10",
    K(KEY_F11) = "F11",
    K(KEY_F12) = "F12",
    K(KEY_F13) = "F13",
    K(KEY_F14) = "F14",
    K(KEY_F15) = "F15",
    K(KEY_F16) = "F16",
    K(KEY_F17) = "F17",
    K(KEY_F18) = "F18",
    K(KEY_F19) = "F19",
    K(KEY_F20) = "F20",
    K(KEY_F21) = "F21",
    K(KEY_F22) = "F22",
    K(KEY_F23) = "F23",
    K(KEY_F24) = "F24",
};

static const struct {
    char name[8];
    KeyCode key;
} other_keys[] = {
    {"tab", KEY_TAB},
    {"enter", KEY_ENTER},
    {"space", KEY_SPACE},
};

UNITTEST {
    static_assert(ARRAYLEN(special_names) == NR_SPECIAL_KEYS);
    CHECK_STRING_ARRAY(special_names);
    CHECK_STRUCT_ARRAY(other_keys, name);

    static_assert(KEY_UNICODE_MAX == UNICODE_MAX_VALID_CODEPOINT);
    static_assert(KEY_MASK + 1 == 1u << 21);
    static_assert((KEY_MASK & MOD_MASK) == 0);
    static_assert(((KEY_MASK | MOD_MASK) & KEYCODE_QUERY_REPLY_BIT) == 0);
    static_assert((KEY_MASK & KEY_SPECIAL_MIN) == KEY_SPECIAL_MIN);
    static_assert((KEY_MASK & KEY_SPECIAL_MAX) == KEY_SPECIAL_MAX);
    static_assert((KEY_MASK & KEY_IGNORE) == KEY_IGNORE);
    static_assert((KEY_MASK & KEYCODE_DETECTED_PASTE) == KEYCODE_DETECTED_PASTE);
    static_assert((KEY_MASK & KEYCODE_BRACKETED_PASTE) == KEYCODE_BRACKETED_PASTE);
}

// https://www.gnu.org/software/emacs/manual/html_node/efaq/Binding-combinations-of-modifiers-and-function-keys.html
static KeyCode modifier_from_char(char c)
{
    switch (c) {
        case 'C': return MOD_CTRL;
        case 'M': return MOD_META;
        case 'S': return MOD_SHIFT;
        case 's': return MOD_SUPER;
        case 'H': return MOD_HYPER;
    }
    return 0;
}

static size_t parse_modifiers(const char *str, KeyCode *modifiersp)
{
    if (str[0] == '^' && str[1] != '\0') {
        *modifiersp = MOD_CTRL;
        return 1;
    }

    KeyCode modifiers = 0;
    size_t i = 0;

    while (1) {
        KeyCode tmp = modifier_from_char(str[i]);
        if (tmp == 0 || str[i + 1] != '-' || modifiers & tmp) {
            break;
        }
        modifiers |= tmp;
        i += 2;
    }

    *modifiersp = modifiers;
    return i;
}

KeyCode keycode_from_str(const char *str)
{
    KeyCode modifiers;
    str += parse_modifiers(str, &modifiers);
    size_t len = strlen(str);
    size_t pos = 0;
    KeyCode ch = u_get_char(str, len, &pos);

    if (u_is_unicode(ch) && pos == len) {
        if (unlikely(u_is_cntrl(ch))) {
            return KEY_NONE;
        }
        if (u_is_ascii_upper(ch)) {
            if (modifiers & MOD_CTRL) {
                // Convert C-A to C-a
                ch = ascii_tolower(ch);
            } else if (modifiers & MOD_META) {
                // Convert M-A to M-S-a
                modifiers |= MOD_SHIFT;
                ch = ascii_tolower(ch);
            }
        }
        return modifiers | ch;
    }

    for (size_t i = 0; i < ARRAYLEN(other_keys); i++) {
        if (ascii_streq_icase(str, other_keys[i].name)) {
            return modifiers | other_keys[i].key;
        }
    }

    for (size_t i = 0; i < ARRAYLEN(special_names); i++) {
        if (ascii_streq_icase(str, special_names[i])) {
            return modifiers | (KEY_SPECIAL_MIN + i);
        }
    }

    return KEY_NONE;
}

static const char *lookup_other_key(KeyCode key)
{
    for (size_t i = 0; i < ARRAYLEN(other_keys); i++) {
        if (key == other_keys[i].key) {
            return other_keys[i].name;
        }
    }
    return NULL;
}

static size_t modifiers_to_str(KeyCode k, char *buf)
{
    static const struct {
        char str[2];
        KeyCode code;
    } mods[] = {
        {"C-", MOD_CTRL},
        {"M-", MOD_META},
        {"S-", MOD_SHIFT},
        {"s-", MOD_SUPER},
        {"H-", MOD_HYPER},
    };

    size_t pos = 0;
    for (size_t i = 0; i < ARRAYLEN(mods); i++) {
        if (k & mods[i].code) {
            pos += copystrn(buf + pos, mods[i].str, sizeof(mods[0].str));
        }
    }

    return pos;
}

// Writes the string representation of `k` into `buf` (which must
// have at least `KEYCODE_STR_BUFSIZE` bytes available) and returns
// the length of the written string.
size_t keycode_to_str(KeyCode k, char buf[static KEYCODE_STR_BUFSIZE])
{
    KeyCode key = keycode_get_key(k);
    KeyCode mask = MOD_MASK | KEY_MASK;
    bool is_key_combo = (k & mask) == k && key <= KEY_SPECIAL_MAX;

    if (unlikely(!is_key_combo)) {
        size_t n = (k & KEYCODE_QUERY_REPLY_BIT)
            ? copyliteral(buf, "QUERY REPLY; 0x")
            : copyliteral(buf, "INVALID; 0x")
        ;
        return n + buf_umax_to_hex_str(k, buf + n, 8);
    }

    const char *name;
    size_t pos = modifiers_to_str(k, buf);

    if (key >= KEY_SPECIAL_MIN) {
        name = special_names[key - KEY_SPECIAL_MIN];
    } else {
        name = lookup_other_key(key);
        if (!name) {
            pos += u_set_char(buf + pos, key);
            buf[pos] = '\0';
            return pos;
        }
    }

    BUG_ON(name[0] == '\0');
    char *end = memccpy(buf + pos, name, '\0', KEYCODE_STR_BUFSIZE - pos);
    BUG_ON(!end);
    BUG_ON(end <= buf);
    return (size_t)(end - buf) - 1;
}
