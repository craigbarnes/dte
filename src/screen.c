#include <string.h>
#include "screen.h"
#include "frame.h"
#include "terminal/cursor.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "terminal/winsize.h"

void set_color(Terminal *term, const ColorScheme *colors, const TermColor *color)
{
    TermColor tmp = *color;
    // NOTE: -2 (keep) is treated as -1 (default)
    if (tmp.fg < 0) {
        tmp.fg = colors->builtin[BC_DEFAULT].fg;
    }
    if (tmp.bg < 0) {
        tmp.bg = colors->builtin[BC_DEFAULT].bg;
    }
    if (same_color(&tmp, &term->obuf.color)) {
        return;
    }
    term_set_color(term, &tmp);
}

void set_builtin_color(Terminal *term, const ColorScheme *colors, BuiltinColorEnum c)
{
    set_color(term, colors, &colors->builtin[c]);
}

void update_cursor_style(EditorState *e)
{
    CursorInputMode mode;
    switch (e->input_mode) {
    case INPUT_NORMAL:
        if (e->buffer->options.overwrite) {
            mode = CURSOR_MODE_OVERWRITE;
        } else {
            mode = CURSOR_MODE_INSERT;
        }
        break;
    case INPUT_COMMAND:
    case INPUT_SEARCH:
        mode = CURSOR_MODE_CMDLINE;
        break;
    default:
        BUG("unhandled input mode");
        return;
    }

    TermCursorStyle style = e->cursor_styles[mode];
    TermCursorStyle def = e->cursor_styles[CURSOR_MODE_DEFAULT];
    if (style.type == CURSOR_KEEP) {
        style.type = def.type;
    }
    if (style.color == COLOR_KEEP) {
        style.color = def.color;
    }

    if (!same_cursor(&style, &e->terminal.obuf.cursor_style)) {
        term_set_cursor_style(&e->terminal, style);
    }

    e->cursor_style_changed = false;
}

void update_term_title(Terminal *term, const Buffer *b, bool set_window_title)
{
    if (!set_window_title || !(term->features & TFLAG_SET_WINDOW_TITLE)) {
        return;
    }

    // FIXME: title must not contain control characters
    TermOutputBuffer *obuf = &term->obuf;
    const char *filename = buffer_filename(b);
    term_add_literal(obuf, "\033]2;");
    term_add_bytes(obuf, filename, strlen(filename));
    term_add_byte(obuf, ' ');
    term_add_byte(obuf, buffer_modified(b) ? '+' : '-');
    term_add_literal(obuf, " dte\033\\");
}

void mask_color(TermColor *color, const TermColor *over)
{
    if (over->fg != COLOR_KEEP) {
        color->fg = over->fg;
    }
    if (over->bg != COLOR_KEEP) {
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

void update_separators(Terminal *term, const ColorScheme *colors, const Frame *frame)
{
    set_builtin_color(term, colors, BC_STATUSLINE);
    frame_for_each_window(frame, print_separator, term);
}

void update_line_numbers(Terminal *term, const ColorScheme *colors, Window *win, bool force)
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
    set_builtin_color(term, colors, BC_LINENUMBER);

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

void update_window_sizes(Terminal *term, Frame *frame)
{
    set_frame_size(frame, term->width, term->height - 1);
    update_window_coordinates(frame);
}

void update_screen_size(Terminal *term, Frame *root_frame)
{
    unsigned int width, height;
    if (!term_get_size(&width, &height)) {
        return;
    }

    // TODO: remove minimum width/height and instead make update_screen()
    // do something sensible when the terminal dimensions are tiny.
    term->width = MAX(width, 3);
    term->height = MAX(height, 3);

    update_window_sizes(term, root_frame);
    LOG_INFO("terminal size: %ux%u", width, height);
}
