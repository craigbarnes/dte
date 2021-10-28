#include <string.h>
#include "screen.h"
#include "editor.h"
#include "frame.h"
#include "terminal/terminal.h"
#include "terminal/winsize.h"

void set_color(TermOutputBuffer *obuf, const TermColor *color)
{
    TermColor tmp = *color;
    // NOTE: -2 (keep) is treated as -1 (default)
    if (tmp.fg < 0) {
        tmp.fg = builtin_colors[BC_DEFAULT].fg;
    }
    if (tmp.bg < 0) {
        tmp.bg = builtin_colors[BC_DEFAULT].bg;
    }
    if (same_color(&tmp, &obuf->color)) {
        return;
    }
    terminal.set_color(obuf, &tmp);
    obuf->color = tmp;
}

void set_builtin_color(TermOutputBuffer *obuf, BuiltinColorEnum c)
{
    set_color(obuf, &builtin_colors[c]);
}

void update_term_title(TermOutputBuffer *obuf, const Buffer *b)
{
    if (
        !editor.options.set_window_title
        || terminal.control_codes.set_title_begin.length == 0
    ) {
        return;
    }

    // FIXME: title must not contain control characters
    const char *filename = buffer_filename(b);
    terminal.put_control_code(obuf, terminal.control_codes.set_title_begin);
    term_add_bytes(obuf, filename, strlen(filename));
    term_add_byte(obuf, ' ');
    term_add_byte(obuf, buffer_modified(b) ? '+' : '-');
    term_add_literal(obuf, " dte");
    terminal.put_control_code(obuf, terminal.control_codes.set_title_end);
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
    TermOutputBuffer *obuf = ud;
    if (win->x + win->w == terminal.width) {
        return;
    }
    for (int y = 0, h = win->h; y < h; y++) {
        term_move_cursor(obuf, win->x + win->w, win->y + y);
        term_add_byte(obuf, '|');
    }
}

void update_separators(TermOutputBuffer *obuf)
{
    set_builtin_color(obuf, BC_STATUSLINE);
    for_each_window_data(print_separator, obuf);
}

void update_line_numbers(TermOutputBuffer *obuf, Window *win, bool force)
{
    const View *v = win->view;
    size_t lines = v->buffer->nl;
    int x = win->x;

    calculate_line_numbers(win);
    long first = v->vy + 1;
    long last = v->vy + win->edit_h;
    if (last > lines) {
        last = lines;
    }

    if (
        !force
        && win->line_numbers.first == first
        && win->line_numbers.last == last
    ) {
        return;
    }

    win->line_numbers.first = first;
    win->line_numbers.last = last;

    char buf[DECIMAL_STR_MAX(unsigned long) + 1];
    size_t width = win->line_numbers.width;
    BUG_ON(width > sizeof(buf));
    BUG_ON(width < LINE_NUMBERS_MIN_WIDTH);
    term_output_reset(obuf, win->x, win->w, 0);
    set_builtin_color(obuf, BC_LINENUMBER);

    for (int y = 0, h = win->edit_h, edit_y = win->edit_y; y < h; y++) {
        unsigned long line = v->vy + y + 1;
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
    set_frame_size(root_frame, terminal.width, terminal.height - 1);
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
    terminal.width = MAX(width, 3);
    terminal.height = MAX(height, 3);

    update_window_sizes();
}
