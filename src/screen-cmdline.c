#include "screen.h"
#include "debug.h"
#include "editor.h"
#include "error.h"
#include "search.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "util/utf8.h"

static void print_message(const char *msg, bool is_error)
{
    enum builtin_color c = BC_COMMANDLINE;
    if (msg[0]) {
        c = is_error ? BC_ERRORMSG : BC_INFOMSG;
    }
    set_builtin_color(c);
    size_t i = 0;
    while (msg[i]) {
        CodePoint u = u_get_char(msg, i + 4, &i);
        if (!buf_put_char(u)) {
            break;
        }
    }
}

void show_message(const char *msg, bool is_error)
{
    buf_reset(0, terminal.width, 0);
    terminal.move_cursor(0, terminal.height - 1);
    print_message(msg, is_error);
    buf_clear_eol();
}

int print_command(char prefix)
{
    size_t i, w;
    CodePoint u;
    int x;

    // Width of characters up to and including cursor position
    w = 1; // ":" (prefix)
    i = 0;
    while (i <= editor.cmdline.pos && i < editor.cmdline.buf.len) {
        u = u_get_char(editor.cmdline.buf.buffer, editor.cmdline.buf.len, &i);
        w += u_char_width(u);
    }
    if (editor.cmdline.pos == editor.cmdline.buf.len) {
        w++;
    }
    if (w > terminal.width) {
        obuf.scroll_x = w - terminal.width;
    }

    set_builtin_color(BC_COMMANDLINE);
    i = 0;
    buf_put_char(prefix);
    x = obuf.x - obuf.scroll_x;
    while (i < editor.cmdline.buf.len) {
        BUG_ON(obuf.x > obuf.scroll_x + obuf.width);
        u = u_get_char(editor.cmdline.buf.buffer, editor.cmdline.buf.len, &i);
        if (!buf_put_char(u)) {
            break;
        }
        if (i <= editor.cmdline.pos) {
            x = obuf.x - obuf.scroll_x;
        }
    }
    return x;
}

void update_command_line(void)
{
    char prefix = ':';
    buf_reset(0, terminal.width, 0);
    terminal.move_cursor(0, terminal.height - 1);
    switch (editor.input_mode) {
    case INPUT_NORMAL:
        print_message(error_ptr, msg_is_error);
        break;
    case INPUT_SEARCH:
        prefix = current_search_direction() == SEARCH_FWD ? '/' : '?';
        // fallthrough
    case INPUT_COMMAND:
        editor.cmdline_x = print_command(prefix);
        break;
    case INPUT_GIT_OPEN:
        break;
    }
    buf_clear_eol();
}
