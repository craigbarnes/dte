#include "screen.h"
#include "error.h"
#include "search.h"

static void print_message(Terminal *term, const ColorScheme *colors, const char *msg, bool is_error)
{
    BuiltinColorEnum c = BC_COMMANDLINE;
    if (msg[0]) {
        c = is_error ? BC_ERRORMSG : BC_INFOMSG;
    }

    TermOutputBuffer *obuf = &term->obuf;
    set_builtin_color(term, colors, c);

    for (size_t i = 0; msg[i]; ) {
        CodePoint u = u_get_char(msg, i + 4, &i);
        if (!term_put_char(obuf, u)) {
            break;
        }
    }
}

void show_message(Terminal *term, const ColorScheme *colors, const char *msg, bool is_error)
{
    term_output_reset(term, 0, term->width, 0);
    term_move_cursor(&term->obuf, 0, term->height - 1);
    print_message(term, colors, msg, is_error);
    term_clear_eol(term);
}

static size_t print_command(Terminal *term, const ColorScheme *colors, const CommandLine *cmdline, char prefix)
{
    const String *buf = &cmdline->buf;
    TermOutputBuffer *obuf = &term->obuf;

    // Width of characters up to and including cursor position
    size_t w = 1; // ":" (prefix)

    for (size_t i = 0; i <= cmdline->pos && i < buf->len; ) {
        CodePoint u = u_get_char(buf->buffer, buf->len, &i);
        w += u_char_width(u);
    }
    if (cmdline->pos == buf->len) {
        w++;
    }
    if (w > term->width) {
        obuf->scroll_x = w - term->width;
    }

    set_builtin_color(term, colors, BC_COMMANDLINE);
    term_put_char(obuf, prefix);

    size_t x = obuf->x - obuf->scroll_x;
    for (size_t i = 0; i < buf->len; ) {
        BUG_ON(obuf->x > obuf->scroll_x + obuf->width);
        CodePoint u = u_get_char(buf->buffer, buf->len, &i);
        if (!term_put_char(obuf, u)) {
            break;
        }
        if (i <= cmdline->pos) {
            x = obuf->x - obuf->scroll_x;
        }
    }

    return x;
}

void update_command_line(EditorState *e)
{
    Terminal *term = &e->terminal;
    char prefix = ':';
    term_output_reset(term, 0, term->width, 0);
    term_move_cursor(&term->obuf, 0, term->height - 1);
    switch (e->input_mode) {
    case INPUT_NORMAL: {
        bool msg_is_error;
        const char *msg = get_msg(&msg_is_error);
        print_message(term, &e->colors, msg, msg_is_error);
        break;
    }
    case INPUT_SEARCH:
        prefix = e->search.reverse ? '?' : '/';
        // Fallthrough
    case INPUT_COMMAND:
        e->cmdline_x = print_command(term, &e->colors, &e->cmdline, prefix);
        break;
    default:
        BUG("unhandled input mode");
    }
    term_clear_eol(term);
}
