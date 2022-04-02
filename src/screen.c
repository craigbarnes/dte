#include <string.h>
#include "screen.h"
#include "frame.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "terminal/winsize.h"

void set_color(EditorState *e, const TermColor *color)
{
    TermColor tmp = *color;
    // NOTE: -2 (keep) is treated as -1 (default)
    if (tmp.fg < 0) {
        tmp.fg = e->colors.builtin[BC_DEFAULT].fg;
    }
    if (tmp.bg < 0) {
        tmp.bg = e->colors.builtin[BC_DEFAULT].bg;
    }
    if (same_color(&tmp, &e->terminal.obuf.color)) {
        return;
    }
    term_set_color(&e->terminal, &tmp);
}

void set_builtin_color(EditorState *e, BuiltinColorEnum c)
{
    set_color(e, &e->colors.builtin[c]);
}

void update_term_title(EditorState *e, const Buffer *b)
{
    if (
        !e->options.set_window_title
        || e->terminal.control_codes.set_title_begin.length == 0
    ) {
        return;
    }

    // FIXME: title must not contain control characters
    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;
    const char *filename = buffer_filename(b);
    term_add_strview(obuf, term->control_codes.set_title_begin);
    term_add_bytes(obuf, filename, strlen(filename));
    term_add_byte(obuf, ' ');
    term_add_byte(obuf, buffer_modified(b) ? '+' : '-');
    term_add_literal(obuf, " dte");
    term_add_strview(obuf, term->control_codes.set_title_end);
}

void mask_color(TermColor *color, const TermColor *over)
{
    if (over->fg != -2) {
        color->fg = over->fg;
    }
    if (over->bg != -2) {
        color->bg = over->bg;
    }
    if (!(over->attr & ATTR_KEEP)) {
        color->attr = over->attr;
    }
}

static void print_separator(Window *win, void *ud)
{
    Terminal *term = ud;
    TermOutputBuffer *obuf = &term->obuf;
    if (win->x + win->w == term->width) {
        return;
    }
    for (int y = 0, h = win->h; y < h; y++) {
        term_move_cursor(obuf, win->x + win->w, win->y + y);
        term_add_byte(obuf, '|');
    }
}

void update_separators(EditorState *e)
{
    set_builtin_color(e, BC_STATUSLINE);
    frame_for_each_window(e->root_frame, print_separator, &e->terminal);
}

void update_line_numbers(EditorState *e, Window *win, bool force)
{
    const View *view = win->view;
    size_t lines = view->buffer->nl;
    int x = win->x;

    calculate_line_numbers(e, win);
    long first = view->vy + 1;
    long last = MIN(view->vy + win->edit_h, lines);

    if (
        !force
        && win->line_numbers.first == first
        && win->line_numbers.last == last
    ) {
        return;
    }

    win->line_numbers.first = first;
    win->line_numbers.last = last;

    TermOutputBuffer *obuf = &e->terminal.obuf;
    char buf[DECIMAL_STR_MAX(unsigned long) + 1];
    size_t width = win->line_numbers.width;
    BUG_ON(width > sizeof(buf));
    BUG_ON(width < LINE_NUMBERS_MIN_WIDTH);
    term_output_reset(&e->terminal, win->x, win->w, 0);
    set_builtin_color(e, BC_LINENUMBER);

    for (int y = 0, h = win->edit_h, edit_y = win->edit_y; y < h; y++) {
        unsigned long line = view->vy + y + 1;
        memset(buf, ' ', width);
        if (line <= lines) {
            size_t i = width - 2;
            do {
                buf[i--] = (line % 10) + '0';
            } while (line /= 10);
        }
        term_move_cursor(obuf, x, edit_y + y);
        term_add_bytes(obuf, buf, width);
    }
}

void update_window_sizes(EditorState *e)
{
    set_frame_size(e->root_frame, e->terminal.width, e->terminal.height - 1);
    update_window_coordinates();
}

void update_screen_size(EditorState *e)
{
    unsigned int width, height;
    if (!term_get_size(&width, &height)) {
        return;
    }

    // TODO: remove minimum width/height and instead make update_screen()
    // do something sensible when the terminal dimensions are tiny.
    e->terminal.width = MAX(width, 3);
    e->terminal.height = MAX(height, 3);

    update_window_sizes(e);
    LOG_INFO("terminal size: %ux%u", width, height);
}
