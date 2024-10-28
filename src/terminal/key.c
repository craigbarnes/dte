#include <string.h>
#include "key.h"
#include "util/array.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/str-util.h"
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

// Writes the string representation of `k` into `buf` (which must
// have at least `KEYCODE_STR_MAX` bytes available) and returns the
// length of the written string.
size_t keycode_to_string(KeyCode k, char *buf)
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

    size_t prefix_len;
    if (k & KEYCODE_QUERY_REPLY_BIT) {
        prefix_len = copyliteral(buf, "QUERY REPLY; 0x");
        goto hex;
    }

    size_t pos = 0;
    for (size_t i = 0; i < ARRAYLEN(mods); i++) {
        if (k & mods[i].code) {
            buf[pos++] = mods[i].prefix;
            buf[pos++] = '-';
        }
    }

    const char *name;
    const KeyCode key = keycode_get_key(k);
    if (u_is_unicode(key)) {
        for (size_t i = 0; i < ARRAYLEN(other_keys); i++) {
            if (key == other_keys[i].key) {
                name = other_keys[i].name;
                goto copy;
            }
        }
        pos += u_set_char(buf + pos, key);
        buf[pos] = '\0';
        return pos;
    }

    if (key >= KEY_SPECIAL_MIN && key <= KEY_SPECIAL_MAX) {
        name = special_names[key - KEY_SPECIAL_MIN];
        goto copy;
    }

    prefix_len = copyliteral(buf, "INVALID; 0x");

hex:
    return prefix_len + buf_umax_to_hex_str(k, buf + prefix_len, 8);

copy:
    BUG_ON(name[0] == '\0');
    char *end = memccpy(buf + pos, name, '\0', KEYCODE_STR_MAX - pos);
    BUG_ON(!end);
    BUG_ON(end <= buf);
    return (size_t)(end - buf) - 1;
}
