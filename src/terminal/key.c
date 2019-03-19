#include <strings.h>
#include <string.h>
#include "key.h"
#include "../util/ascii.h"
#include "../util/string.h"
#include "../util/uchar.h"

// Note: these strings must be kept in sync with the enum in key.h
static const char special_names[][8] = {
    "insert",
    "delete",
    "up",
    "down",
    "right",
    "left",
    "pgdown",
    "end",
    "pgup",
    "home",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
};
static_assert(ARRAY_COUNT(special_names) == NR_SPECIAL_KEYS);

static size_t parse_modifiers(const char *const str, KeyCode *modifiersp)
{
    KeyCode modifiers = 0;
    size_t i = 0;

    while (true) {
        const unsigned char ch = ascii_toupper(str[i]);
        if (ch == '^' && str[i + 1] != 0) {
            modifiers |= MOD_CTRL;
            i++;
        } else if (ch == 'C' && str[i + 1] == '-') {
            modifiers |= MOD_CTRL;
            i += 2;
        } else if (ch == 'M' && str[i + 1] == '-') {
            modifiers |= MOD_META;
            i += 2;
        } else if (ch == 'S' && str[i + 1] == '-') {
            modifiers |= MOD_SHIFT;
            i += 2;
        } else {
            break;
        }
    }
    *modifiersp = modifiers;
    return i;
}

bool parse_key(KeyCode *key, const char *str)
{
    KeyCode modifiers;
    str += parse_modifiers(str, &modifiers);
    const size_t len = strlen(str);

    size_t i = 0;
    KeyCode ch = u_get_char(str, len, &i);
    if (u_is_unicode(ch) && i == len) {
        if (modifiers == MOD_CTRL) {
            // Normalize
            switch (ch) {
            case 'i':
            case 'I':
                ch = '\t';
                modifiers = 0;
                break;
            case 'm':
            case 'M':
                ch = KEY_ENTER;
                modifiers = 0;
                break;
            }
        }
        *key = modifiers | ch;
        return true;
    }
    if (!strcasecmp(str, "space")) {
        *key = modifiers | ' ';
        return true;
    }
    if (!strcasecmp(str, "tab")) {
        *key = modifiers | '\t';
        return true;
    }
    if (!strcasecmp(str, "enter")) {
        *key = modifiers | KEY_ENTER;
        return true;
    }

    for (i = 0; i < NR_SPECIAL_KEYS; i++) {
        if (!strcasecmp(str, special_names[i])) {
            *key = modifiers | (KEY_SPECIAL_MIN + i);
            return true;
        }
    }

    return false;
}

char *key_to_string(KeyCode k)
{
    String buf = STRING_INIT;

    if (k & MOD_CTRL) {
        string_add_literal(&buf, "C-");
    }
    if (k & MOD_META) {
        string_add_literal(&buf, "M-");
    }
    if (k & MOD_SHIFT) {
        string_add_literal(&buf, "S-");
    }

    const KeyCode key = keycode_get_key(k);
    if (u_is_unicode(key)) {
        switch (key) {
        case '\t':
            string_add_literal(&buf, "tab");
            break;
        case KEY_ENTER:
            string_add_literal(&buf, "enter");
            break;
        case ' ':
            string_add_literal(&buf, "space");
            break;
        default:
            // <0x20 or 0x7f shouldn't be possible
            string_add_ch(&buf, key);
        }
    } else if (key >= KEY_SPECIAL_MIN && key <= KEY_SPECIAL_MAX) {
        string_add_str(&buf, special_names[key - KEY_SPECIAL_MIN]);
    } else if (key == KEY_PASTE) {
        string_add_literal(&buf, "paste");
    } else {
        string_add_literal(&buf, "???");
    }

    return string_steal_cstring(&buf);
}

bool key_to_ctrl(KeyCode k, unsigned char *byte)
{
    if (keycode_get_modifiers(k) != MOD_CTRL) {
        return false;
    }

    const KeyCode key = keycode_get_key(k);
    if (key >= '@' && key <= '_') {
        *byte = key & ~0x40;
        return true;
    } else if (key == '?') {
        *byte = 0x7f;
        return true;
    }

    return false;
}
