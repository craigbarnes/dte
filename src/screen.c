#include <string.h>
#include "screen.h"
#include "frame.h"
#include "terminal/cursor.h"
#include "terminal/ioctl.h"
#include "util/log.h"

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

static void update_cursor_style(EditorState *e)
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

    e->cursor_style_changed = false;
    if (!same_cursor(&style, &e->terminal.obuf.cursor_style)) {
        term_set_cursor_style(&e->terminal, style);
    }
}

void update_term_title(Terminal *term, const Buffer *buffer, bool set_window_title)
{
    if (!set_window_title || !(term->features & TFLAG_SET_WINDOW_TITLE)) {
        return;
    }

    // FIXME: title must not contain control characters
    TermOutputBuffer *obuf = &term->obuf;
    const char *filename = buffer_filename(buffer);
    term_add_literal(obuf, "\033]2;");
    term_add_bytes(obuf, filename, strlen(filename));
    term_add_byte(obuf, ' ');
    term_add_byte(obuf, buffer_modified(buffer) ? '+' : '-');
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

void restore_cursor(EditorState *e)
{
    unsigned int x, y;
    switch (e->input_mode) {
    case INPUT_NORMAL:
        x = e->window->edit_x + e->view->cx_display - e->view->vx;
        y = e->window->edit_y + e->view->cy - e->view->vy;
        break;
    case INPUT_COMMAND:
    case INPUT_SEARCH:
        x = e->cmdline_x;
        y = e->terminal.height - 1;
        break;
    default:
        BUG("unhandled input mode");
    }
    term_move_cursor(&e->terminal.obuf, x, y);
}

static void clear_update_tabbar(Window *window, void* UNUSED_ARG(data))
{
    window->update_tabbar = false;
}

void end_update(EditorState *e)
{
    Terminal *term = &e->terminal;
    restore_cursor(e);
    term_show_cursor(term);
    term_end_sync_update(term);
    term_output_flush(&term->obuf);

    e->buffer->changed_line_min = LONG_MAX;
    e->buffer->changed_line_max = -1;
    frame_for_each_window(e->root_frame, clear_update_tabbar, NULL);
}

void start_update(Terminal *term)
{
    term_begin_sync_update(term);
    term_hide_cursor(term);
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
    // do something sensible when the terminal dimensions are tiny
    term->width = MAX(width, 3);
    term->height = MAX(height, 3);

    update_window_sizes(term, root_frame);
    LOG_INFO("terminal size: %ux%u", width, height);
}

NOINLINE
void normal_update(EditorState *e)
{
    Terminal *term = &e->terminal;
    start_update(term);
    update_term_title(term, e->buffer, e->options.set_window_title);
    update_all_windows(e);
    update_command_line(e);
    update_cursor_style(e);
    end_update(e);
}

void update_screen(EditorState *e, const ScreenState *s)
{
    if (e->everything_changed) {
        e->everything_changed = false;
        normal_update(e);
        return;
    }

    Buffer *buffer = e->buffer;
    View *view = e->view;
    view_update_cursor_x(view);
    view_update_cursor_y(view);
    view_update(view, e->options.scroll_margin);

    if (s->id == buffer->id) {
        if (s->vx != view->vx || s->vy != view->vy) {
            mark_all_lines_changed(buffer);
        } else {
            // Because of trailing whitespace highlighting and highlighting
            // current line in different color, the lines cy (old cursor y) and
            // view->cy need to be updated. Always update at least current line.
            buffer_mark_lines_changed(buffer, s->cy, view->cy);
        }
        if (s->is_modified != buffer_modified(buffer)) {
            mark_buffer_tabbars_changed(buffer);
        }
    } else {
        e->window->update_tabbar = true;
        mark_all_lines_changed(buffer);
    }

    start_update(&e->terminal);
    if (e->window->update_tabbar) {
        update_term_title(&e->terminal, e->buffer, e->options.set_window_title);
    }
    update_buffer_windows(e, buffer);
    update_command_line(e);
    if (e->cursor_style_changed) {
        update_cursor_style(e);
    }
    end_update(e);
}
