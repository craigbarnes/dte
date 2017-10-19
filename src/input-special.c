#include "input-special.h"
#include "uchar.h"
#include "common.h"

enum input_special {
    // Not inputting special characters
    INPUT_SPECIAL_NONE,

    // Not known yet (just started by hitting ^V)
    INPUT_SPECIAL_UNKNOWN,

    // Accept any value 0-255 (3 octal digits)
    INPUT_SPECIAL_OCT,

    // Accept any value 0-255 (3 decimal digits)
    INPUT_SPECIAL_DEC,

    // Accept any value 0-255 (2 hexadecimal digits)
    INPUT_SPECIAL_HEX,

    // Accept any valid unicode value (6 hexadecimal digits)
    INPUT_SPECIAL_UNICODE,
};

static enum input_special input_special;
static struct {
    int base;
    int max_chars;
    int value;
    int nr;
} raw_input;

void special_input_activate(void)
{
    input_special = INPUT_SPECIAL_UNKNOWN;
}

static void keypress(Key key, char *buf, int *count)
{
    if (key == KEY_PASTE) {
        term_discard_paste();
        input_special = INPUT_SPECIAL_NONE;
        return;
    }
    if (input_special == INPUT_SPECIAL_UNKNOWN) {
        raw_input.value = 0;
        raw_input.nr = 0;
        if (key >= '0' && key <= '9') {
            input_special = INPUT_SPECIAL_DEC;
            raw_input.base = 10;
            raw_input.max_chars = 3;
        } else {
            unsigned char byte;
            switch (key) {
            case 'o':
            case 'O':
                input_special = INPUT_SPECIAL_OCT;
                raw_input.base = 8;
                raw_input.max_chars = 3;
                break;
            case 'x':
            case 'X':
                input_special = INPUT_SPECIAL_HEX;
                raw_input.base = 16;
                raw_input.max_chars = 2;
                break;
            case 'u':
            case 'U':
                input_special = INPUT_SPECIAL_UNICODE;
                raw_input.base = 16;
                raw_input.max_chars = 6;
                break;
            default:
                if (key_to_ctrl(key, &byte)) {
                    buf[0] = byte;
                    *count = 1;
                } else if (u_is_unicode(key)) {
                    size_t idx = 0;
                    u_set_char_raw(buf, &idx, key);
                    *count = idx;
                }
                input_special = INPUT_SPECIAL_NONE;
            }
            return;
        }
    }

    if (key == CTRL('H') || key == CTRL('?')) {
        if (raw_input.nr) {
            raw_input.value /= raw_input.base;
            raw_input.nr--;
        }
        return;
    }
    if (!u_is_unicode(key)) {
        input_special = INPUT_SPECIAL_NONE;
        return;
    }

    if (key != KEY_ENTER) {
        int n = hex_decode(key);

        if (n < 0 || n >= raw_input.base) {
            input_special = INPUT_SPECIAL_NONE;
            return;
        }
        raw_input.value *= raw_input.base;
        raw_input.value += n;
        if (++raw_input.nr < raw_input.max_chars) {
            return;
        }
    }

    if (
        input_special == INPUT_SPECIAL_UNICODE
        && u_is_unicode(raw_input.value)
    ) {
        size_t idx = 0;
        u_set_char_raw(buf, &idx, raw_input.value);
        // TODO: count should really be changed to size_t
        *count = (int)idx;
    }
    if (
        input_special != INPUT_SPECIAL_UNICODE
        && raw_input.value <= 255
    ) {
        buf[0] = raw_input.value;
        *count = 1;
    }
    input_special = INPUT_SPECIAL_NONE;
}

bool special_input_keypress(Key key, char *buf, int *count)
{
    *count = 0;
    if (input_special == INPUT_SPECIAL_NONE) {
        return false;
    }
    keypress(key, buf, count);
    return true;
}

bool special_input_misc_status(char *status, size_t status_maxlen)
{
    static const char hex_tab[16] = "0123456789abcdef";
    int i, value = raw_input.value;
    const char *str = "";
    char buf[7];

    if (input_special == INPUT_SPECIAL_NONE) {
        return false;
    }

    if (input_special == INPUT_SPECIAL_UNKNOWN) {
        strncpy(status, "[Insert special]", status_maxlen);
        return true;
    }

    for (i = 0; i < raw_input.nr; i++) {
        buf[raw_input.nr - i - 1] = hex_tab[value % raw_input.base];
        value /= raw_input.base;
    }
    while (i < raw_input.max_chars) {
        buf[i++] = ' ';
    }
    buf[i] = 0;

    switch (input_special) {
    case INPUT_SPECIAL_NONE:
        break;
    case INPUT_SPECIAL_UNKNOWN:
        break;
    case INPUT_SPECIAL_OCT:
        str = "Oct";
        break;
    case INPUT_SPECIAL_DEC:
        str = "Dec";
        break;
    case INPUT_SPECIAL_HEX:
        str = "Hex";
        break;
    case INPUT_SPECIAL_UNICODE:
        str = "Unicode, hex";
        break;
    }

    snprintf(status, status_maxlen, "[%s <%s>]", str, buf);
    return true;
}
