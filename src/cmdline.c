#include "cmdline.h"
#include "common.h"
#include "editor.h"
#include "history.h"
#include "terminal/input.h"
#include "util/ascii.h"
#include "util/utf8.h"

static void cmdline_delete(CommandLine *c)
{
    size_t pos = c->pos;
    size_t len = 1;

    if (pos == c->buf.len) {
        return;
    }

    u_get_char(c->buf.buffer, c->buf.len, &pos);
    len = pos - c->pos;
    string_remove(&c->buf, c->pos, len);
}

static void cmdline_backspace(CommandLine *c)
{
    if (c->pos) {
        u_prev_char(c->buf.buffer, &c->pos);
        cmdline_delete(c);
    }
}

static void cmdline_erase_word(CommandLine *c)
{
    size_t i = c->pos;

    if (i == 0) {
        return;
    }

    // open /path/to/file^W => open /path/to/

    // erase whitespace
    while (i && ascii_isspace(c->buf.buffer[i - 1])) {
        i--;
    }

    // erase non-word bytes
    while (i && !is_word_byte(c->buf.buffer[i - 1])) {
        i--;
    }

    // erase word bytes
    while (i && is_word_byte(c->buf.buffer[i - 1])) {
        i--;
    }

    string_remove(&c->buf, i, c->pos - i);
    c->pos = i;
}

static void cmdline_delete_word(CommandLine *c)
{
    const unsigned char *buf = c->buf.buffer;
    const size_t len = c->buf.len;
    size_t i = c->pos;

    if (i == len) {
        return;
    }

    while (i < len && is_word_byte(buf[i])) {
        i++;
    }

    while (i < len && !is_word_byte(buf[i])) {
        i++;
    }

    string_remove(&c->buf, c->pos, i - c->pos);
}

static void cmdline_delete_bol(CommandLine *c)
{
    string_remove(&c->buf, 0, c->pos);
    c->pos = 0;
}

static void cmdline_delete_eol(CommandLine *c)
{
    c->buf.len = c->pos;
}

static void cmdline_prev_char(CommandLine *c)
{
    if (c->pos) {
        u_prev_char(c->buf.buffer, &c->pos);
    }
}

static void cmdline_next_char(CommandLine *c)
{
    if (c->pos < c->buf.len) {
        u_get_char(c->buf.buffer, c->buf.len, &c->pos);
    }
}

static void cmdline_next_word(CommandLine *c)
{
    const unsigned char *buf = c->buf.buffer;
    const size_t len = c->buf.len;
    size_t i = c->pos;

    while (i < len && is_word_byte(buf[i])) {
        i++;
    }

    while (i < len && !is_word_byte(buf[i])) {
        i++;
    }

    c->pos = i;
}

static void cmdline_prev_word(CommandLine *c)
{
    switch (c->pos) {
    case 1:
        c->pos = 0;
        return;
    case 0:
        return;
    }

    const unsigned char *const buf = c->buf.buffer;
    size_t i = c->pos - 1;

    while (i > 0 && !is_word_byte(buf[i])) {
        i--;
    }

    while (i > 0 && is_word_byte(buf[i])) {
        i--;
    }

    if (i > 0) {
        i++;
    }

    c->pos = i;
}

static void cmdline_insert_bytes(CommandLine *c, const char *buf, size_t size)
{
    string_make_space(&c->buf, c->pos, size);
    for (size_t i = 0; i < size; i++) {
        c->buf.buffer[c->pos++] = buf[i];
    }
}

static void cmdline_insert_paste(CommandLine *c)
{
    size_t size;
    char *text = term_read_paste(&size);
    for (size_t i = 0; i < size; i++) {
        if (text[i] == '\n') {
            text[i] = ' ';
        }
    }
    cmdline_insert_bytes(c, text, size);
    free(text);
}

static void set_text(CommandLine *c, const char *text)
{
    string_clear(&c->buf);
    string_add_str(&c->buf, text);
    c->pos = strlen(text);
}

void cmdline_clear(CommandLine *c)
{
    string_clear(&c->buf);
    c->pos = 0;
    c->search_pos = -1;
}

void cmdline_set_text(CommandLine *c, const char *text)
{
    set_text(c, text);
    c->search_pos = -1;
}

CommandLineResult cmdline_handle_key (
    CommandLine *c,
    PointerArray *history,
    KeyCode key
) {
    if (key <= KEY_UNICODE_MAX) {
        c->pos += string_insert_ch(&c->buf, c->pos, key);
        return CMDLINE_KEY_HANDLED;
    }
    switch (key) {
    case CTRL('['): // ESC
    case CTRL('C'):
    case CTRL('G'):
        cmdline_clear(c);
        return CMDLINE_CANCEL;
    case CTRL('D'):
        cmdline_delete(c);
        goto reset_search_pos;
    case CTRL('K'):
        cmdline_delete_eol(c);
        goto reset_search_pos;
    case CTRL('H'):
    case CTRL('?'):
        if (c->buf.len > 0) {
            cmdline_backspace(c);
        }
        goto reset_search_pos;
    case CTRL('U'):
        cmdline_delete_bol(c);
        goto reset_search_pos;
    case CTRL('W'):
    case MOD_META | MOD_CTRL | 'H':
    case MOD_META | MOD_CTRL | '?':
        cmdline_erase_word(c);
        goto reset_search_pos;
    case MOD_CTRL | KEY_DELETE:
    case MOD_META | KEY_DELETE:
    case MOD_META | 'd':
        cmdline_delete_word(c);
        goto reset_search_pos;

    case CTRL('A'):
        c->pos = 0;
        goto handled;
    case CTRL('B'):
        cmdline_prev_char(c);
        goto handled;
    case CTRL('E'):
        c->pos = c->buf.len;
        goto handled;
    case CTRL('F'):
        cmdline_next_char(c);
        goto handled;
    case KEY_DELETE:
        cmdline_delete(c);
        goto reset_search_pos;

    case KEY_LEFT:
        cmdline_prev_char(c);
        goto handled;
    case KEY_RIGHT:
        cmdline_next_char(c);
        goto handled;
    case CTRL(KEY_LEFT):
    case MOD_META | 'b':
        cmdline_prev_word(c);
        goto handled;
    case CTRL(KEY_RIGHT):
    case MOD_META | 'f':
        cmdline_next_word(c);
        goto handled;

    case KEY_HOME:
    case MOD_META | KEY_LEFT:
        c->pos = 0;
        goto handled;
    case KEY_END:
    case MOD_META | KEY_RIGHT:
        c->pos = c->buf.len;
        goto handled;
    case KEY_UP:
        if (history == NULL) {
            return CMDLINE_UNKNOWN_KEY;
        }
        if (c->search_pos < 0) {
            free(c->search_text);
            c->search_text = string_cstring(&c->buf);
            c->search_pos = history->count;
        }
        if (history_search_forward(history, &c->search_pos, c->search_text)) {
            set_text(c, history->ptrs[c->search_pos]);
        }
        goto handled;
    case KEY_DOWN:
        if (history == NULL) {
            return CMDLINE_UNKNOWN_KEY;
        }
        if (c->search_pos < 0) {
            goto handled;
        }
        if (history_search_backward(history, &c->search_pos, c->search_text)) {
            set_text(c, history->ptrs[c->search_pos]);
        } else {
            set_text(c, c->search_text);
            c->search_pos = -1;
        }
        goto handled;
    case KEY_PASTE:
        cmdline_insert_paste(c);
        goto reset_search_pos;
    default:
        return CMDLINE_UNKNOWN_KEY;
    }

reset_search_pos:
    c->search_pos = -1;
handled:
    return CMDLINE_KEY_HANDLED;
}
