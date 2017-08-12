#include "cmdline.h"
#include "history.h"
#include "editor.h"
#include "common.h"
#include "uchar.h"
#include "input-special.h"

static void cmdline_delete(CommandLine *c)
{
    long pos = c->pos;
    long len = 1;

    if (pos == c->buf.len)
        return;

    u_get_char(c->buf.buffer, c->buf.len, &pos);
    len = pos - c->pos;
    gbuf_remove(&c->buf, c->pos, len);
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
    int i = c->pos;

    if (!c->pos)
        return;

    // open /path/to/file^W => open /path/to/

    // erase whitespace
    while (i && isspace(c->buf.buffer[i - 1]))
        i--;

    // erase non-word bytes
    while (i && !is_word_byte(c->buf.buffer[i - 1]))
        i--;

    // erase word bytes
    while (i && is_word_byte(c->buf.buffer[i - 1]))
        i--;

    gbuf_remove(&c->buf, i, c->pos - i);
    c->pos = i;
}

static void cmdline_delete_bol(CommandLine *c)
{
    gbuf_remove(&c->buf, 0, c->pos);
    c->pos = 0;
}

static void cmdline_delete_eol(CommandLine *c)
{
    c->buf.len = c->pos;
}

static void cmdline_prev_char(CommandLine *c)
{
    if (c->pos)
        u_prev_char(c->buf.buffer, &c->pos);
}

static void cmdline_next_char(CommandLine *c)
{
    if (c->pos < c->buf.len)
        u_get_char(c->buf.buffer, c->buf.len, &c->pos);
}

static void cmdline_insert_bytes(CommandLine *c, const char *buf, int size)
{
    int i;

    gbuf_make_space(&c->buf, c->pos, size);
    for (i = 0; i < size; i++)
        c->buf.buffer[c->pos++] = buf[i];
}

static void cmdline_insert_paste(CommandLine *c)
{
    long size, i;
    char *text = term_read_paste(&size);

    for (i = 0; i < size; i++) {
        if (text[i] == '\n')
            text[i] = ' ';
    }
    cmdline_insert_bytes(c, text, size);
    free(text);
}

static void set_text(CommandLine *c, const char *text)
{
    gbuf_clear(&c->buf);
    gbuf_add_str(&c->buf, text);
    c->pos = strlen(text);
}

void cmdline_clear(CommandLine *c)
{
    gbuf_clear(&c->buf);
    c->pos = 0;
    c->search_pos = -1;
}

void cmdline_set_text(CommandLine *c, const char *text)
{
    set_text(c, text);
    c->search_pos = -1;
}

int cmdline_handle_key(CommandLine *c, PointerArray *history, int key)
{
    char buf[4];
    int count;

    if (special_input_keypress(key, buf, &count)) {
        // \n is not allowed in command line because
        // command/search history file would break
        if (count && buf[0] != '\n')
            cmdline_insert_bytes(&cmdline, buf, count);
        c->search_pos = -1;
        return 1;
    }
    if (key <= KEY_UNICODE_MAX) {
        c->pos += gbuf_insert_ch(&c->buf, c->pos, key);
        return 1;
    }
    switch (key) {
    case CTRL('['): // ESC
    case CTRL('C'):
        cmdline_clear(c);
        return CMDLINE_CANCEL;
    case CTRL('D'):
        cmdline_delete(c);
        break;
    case CTRL('K'):
        cmdline_delete_eol(c);
        break;
    case CTRL('H'):
    case CTRL('?'):
        if (c->buf.len == 0)
            return CMDLINE_CANCEL;
        cmdline_backspace(c);
        break;
    case CTRL('U'):
        cmdline_delete_bol(c);
        break;
    case CTRL('V'):
        special_input_activate();
        break;
    case CTRL('W'):
        cmdline_erase_word(c);
        break;

    case CTRL('A'):
        c->pos = 0;
        return 1;
    case CTRL('B'):
        cmdline_prev_char(c);
        return 1;
    case CTRL('E'):
        c->pos = c->buf.len;
        return 1;
    case CTRL('F'):
        cmdline_next_char(c);
        return 1;
    case KEY_DELETE:
        cmdline_delete(c);
        break;

    case KEY_LEFT:
        cmdline_prev_char(c);
        return 1;
    case KEY_RIGHT:
        cmdline_next_char(c);
        return 1;
    case KEY_HOME:
        c->pos = 0;
        return 1;
    case KEY_END:
        c->pos = c->buf.len;
        return 1;
    case KEY_UP:
        if (history == NULL)
            return 0;
        if (c->search_pos < 0) {
            free(c->search_text);
            c->search_text = gbuf_cstring(&c->buf);
            c->search_pos = history->count;
        }
        if (history_search_forward(history, &c->search_pos, c->search_text))
            set_text(c, history->ptrs[c->search_pos]);
        return 1;
    case KEY_DOWN:
        if (history == NULL)
            return 0;
        if (c->search_pos < 0)
            return 1;
        if (history_search_backward(history, &c->search_pos, c->search_text)) {
            set_text(c, history->ptrs[c->search_pos]);
        } else {
            set_text(c, c->search_text);
            c->search_pos = -1;
        }
        return 1;
    case KEY_PASTE:
        cmdline_insert_paste(c);
        break;
    default:
        return 0;
    }
    c->search_pos = -1;
    return 1;
}
