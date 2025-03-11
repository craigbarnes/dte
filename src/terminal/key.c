#include <string.h>
#include "key.h"
#include "util/array.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/str-util.h"
#include "util/unicode.h"
#include "util/utf8.h"

#define KEY(keyname) [KEY_ ## keyname - KEY_SPECIAL_MIN]

static const char special_names[][10] = {
    KEY(ESCAPE) = "escape",
    KEY(BACKSPACE) = "backspace",
    KEY(INSERT) = "insert",
    KEY(DELETE) = "delete",
    KEY(HOME) = "home",
    KEY(END) = "end",
    KEY(PAGE_UP) = "pgup",
    KEY(PAGE_DOWN) = "pgdown",
    KEY(UP) = "up",
    KEY(DOWN) = "down",
    KEY(RIGHT) = "right",
    KEY(LEFT) = "left",
    KEY(BEGIN) = "begin",
    KEY(SCROLL_LOCK) = "scrlock",
    KEY(PRINT_SCREEN) = "print",
    KEY(PAUSE) = "pause",
    KEY(MENU) = "menu",
    KEY(F1) = "F1",
    KEY(F2) = "F2",
    KEY(F3) = "F3",
    KEY(F4) = "F4",
    KEY(F5) = "F5",
    KEY(F6) = "F6",
    KEY(F7) = "F7",
    KEY(F8) = "F8",
    KEY(F9) = "F9",
    KEY(F10) = "F10",
    KEY(F11) = "F11",
    KEY(F12) = "F12",
    KEY(F13) = "F13",
    KEY(F14) = "F14",
    KEY(F15) = "F15",
    KEY(F16) = "F16",
    KEY(F17) = "F17",
    KEY(F18) = "F18",
    KEY(F19) = "F19",
    KEY(F20) = "F20",
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
    static_assert((KEY_MASK & MOD_MASK & KEYCODE_QUERY_REPLY_BIT) == 0);
    static_assert((KEY_MASK & KEY_SPECIAL_MIN) == KEY_SPECIAL_MIN);
    static_assert((KEY_MASK & KEY_SPECIAL_MAX) == KEY_SPECIAL_MAX);
    static_assert((KEY_MASK & KEY_IGNORE) == KEY_IGNORE);
    static_assert((KEY_MASK & KEYCODE_DETECTED_PASTE) == KEYCODE_DETECTED_PASTE);
    static_assert((KEY_MASK & KEYCODE_BRACKETED_PASTE) == KEYCODE_BRACKETED_PASTE);
}

static size_t parse_modifiers(const char *str, KeyCode *modifiersp)
{
    if (str[0] == '^' && str[1] != '\0') {
        *modifiersp = MOD_CTRL;
        return 1;
    }

    KeyCode modifiers = 0;
    size_t i = 0;

    // https://www.gnu.org/software/emacs/manual/html_node/efaq/Binding-combinations-of-modifiers-and-function-keys.html
    while (1) {
        KeyCode tmp;
        switch (str[i]) {
        case 'C':
            tmp = MOD_CTRL;
            break;
        case 'M':
            tmp = MOD_META;
            break;
        case 'S':
            tmp = MOD_SHIFT;
            break;
        case 's':
            tmp = MOD_SUPER;
            break;
        case 'H':
            tmp = MOD_HYPER;
            break;
        default:
            goto end;
        }
        if (str[i + 1] != '-' || modifiers & tmp) {
            goto end;
        }
        modifiers |= tmp;
        i += 2;
    }

end:
    *modifiersp = modifiers;
    return i;
}

KeyCode parse_key_string(const char *str)
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

// Writes the string representation of `k` into `buf` (which must
// have at least `KEYCODE_STR_BUFSIZE` bytes available) and returns
// the length of the written string.
size_t keycode_to_string(KeyCode k, char buf[KEYCODE_STR_BUFSIZE])
{
    static const struct {
        char prefix;
        KeyCode code;
    } mods[] = {
        {'C', MOD_CTRL},
        {'M', MOD_META},
        {'S', MOD_SHIFT},
        {'s', MOD_SUPER},
        {'H', MOD_HYPER},
    };

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

    size_t pos = 0;
    for (size_t i = 0; i < ARRAYLEN(mods); i++) {
        if (k & mods[i].code) {
            buf[pos++] = mods[i].prefix;
            buf[pos++] = '-';
        }
    }

    const char *name;
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
