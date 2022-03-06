#include <string.h>
#include "screen.h"
#include "editor.h"
#include "frame.h"
#include "terminal/ecma48.h"
#include "terminal/terminal.h"
#include "terminal/winsize.h"

void set_color(Terminal *term, const TermColor *color)
{
    TermOutputBuffer *obuf = &term->obuf;
    TermColor tmp = *color;
    // NOTE: -2 (keep) is treated as -1 (default)
    if (tmp.fg < 0) {
        tmp.fg = editor.colors.builtin[BC_DEFAULT].fg;
    }
    if (tmp.bg < 0) {
        tmp.bg = editor.colors.builtin[BC_DEFAULT].bg;
    }
    if (same_color(&tmp, &obuf->color)) {
        return;
    }
    term_set_color(term, &tmp);
    obuf->color = tmp;
}

void set_builtin_color(Terminal *term, BuiltinColorEnum c)
{
    set_color(term, &editor.colors.builtin[c]);
}

void update_term_title(Terminal *term, const Buffer *b)
{
    if (
        !editor.options.set_window_title
        || term->control_codes.set_title_begin.length == 0
    ) {
        return;
    }

    // FIXME: title must not contain control characters
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

void update_separators(Terminal *term)
{
    set_builtin_color(term, BC_STATUSLINE);
    frame_for_each_window(editor.root_frame, print_separator, term);
}

void update_line_numbers(Terminal *term, Window *win, bool force)
{
    const View *view = win->view;
    size_t lines = view->buffer->nl;
    int x = win->x;

    calculate_line_numbers(win);
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

    TermOutputBuffer *obuf = &term->obuf;
    char buf[DECIMAL_STR_MAX(unsigned long) + 1];
    size_t width = win->line_numbers.width;
    BUG_ON(width > sizeof(buf));
    BUG_ON(width < LINE_NUMBERS_MIN_WIDTH);
    term_output_reset(term, win->x, win->w, 0);
    set_builtin_color(term, BC_LINENUMBER);

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

void update_window_sizes(void)
{
    const Terminal *term = &editor.terminal;
    set_frame_size(editor.root_frame, term->width, term->height - 1);
    update_window_coordinates();
}

void update_screen_size(void)
{
    unsigned int width, height;
    if (!term_get_size(&width, &height)) {
        return;
    }

    // TODO: remove minimum width/height and instead make update_screen()
    // do something sensible when the terminal dimensions are tiny.
    editor.terminal.width = MAX(width, 3);
    editor.terminal.height = MAX(height, 3);

    update_window_sizes();
}
