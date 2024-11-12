#include <string.h>
#include "ui.h"
#include "error.h"
#include "frame.h"
#include "terminal/cursor.h"
#include "terminal/ioctl.h"
#include "util/log.h"

void set_style(Terminal *term, const StyleMap *styles, const TermStyle *style)
{
    TermStyle tmp = *style;
    // NOTE: -2 (keep) is treated as -1 (default)
    if (tmp.fg < 0) {
        tmp.fg = styles->builtin[BSE_DEFAULT].fg;
    }
    if (tmp.bg < 0) {
        tmp.bg = styles->builtin[BSE_DEFAULT].bg;
    }
    if (same_style(&tmp, &term->obuf.style)) {
        return;
    }
    term_set_style(term, tmp);
}

void set_builtin_style(Terminal *term, const StyleMap *styles, BuiltinStyleEnum s)
{
    set_style(term, styles, &styles->builtin[s]);
}

static void update_cursor_style(EditorState *e)
{
    CursorInputMode mode;
    if (e->mode->cmds == &normal_commands) {
        bool overwrite = e->buffer->options.overwrite;
        mode = overwrite ? CURSOR_MODE_OVERWRITE : CURSOR_MODE_INSERT;
    } else {
        mode = CURSOR_MODE_CMDLINE;
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
}

void update_term_title(Terminal *term, const Buffer *buffer, bool set_window_title)
{
    if (!set_window_title || !(term->features & TFLAG_SET_WINDOW_TITLE)) {
        return;
    }

    // FIXME: title must not contain control characters
    TermOutputBuffer *obuf = &term->obuf;
    const char *filename = buffer_filename(buffer);
    term_put_literal(obuf, "\033]2;");
    term_put_bytes(obuf, filename, strlen(filename));
    term_put_byte(obuf, ' ');
    term_put_byte(obuf, buffer_modified(buffer) ? '+' : '-');
    term_put_literal(obuf, " dte\033\\");
}

static void restore_cursor(EditorState *e)
{
    unsigned int x, y;
    if (e->mode->cmds == &normal_commands) {
        x = e->window->edit_x + e->view->cx_display - e->view->vx;
        y = e->window->edit_y + e->view->cy - e->view->vy;
    } else {
        x = e->cmdline_x;
        y = e->terminal.height - 1;
    }
    term_move_cursor(&e->terminal.obuf, x, y);
}

static void clear_update_tabbar(Window *window, void* UNUSED_ARG(data))
{
    window->update_tabbar = false;
}

static void end_update(EditorState *e)
{
    Terminal *term = &e->terminal;
    term_end_sync_update(term);
    term_output_flush(&term->obuf);

    e->buffer->changed_line_min = LONG_MAX;
    e->buffer->changed_line_max = -1;

    const Frame *root = e->root_frame;
    if (root->window) {
        // Only 1 window to update
        clear_update_tabbar(root->window, NULL);
    } else {
        frame_for_each_window(root, clear_update_tabbar, NULL);
    }
}

static void start_update(Terminal *term)
{
    term_begin_sync_update(term);
    term_hide_cursor(term);
}

void update_window_sizes(Terminal *term, Frame *frame)
{
    frame_set_size(frame, term->width, term->height - 1);
    update_window_coordinates(frame);
}

static bool is_root_frame(const Frame *frame)
{
    return !frame->parent;
}

void update_screen_size(Terminal *term, Frame *root_frame)
{
    BUG_ON(!is_root_frame(root_frame));
    unsigned int w, h;
    if (!term_get_size(&w, &h) || (w == term->width && h == term->height)) {
        return;
    }

    // TODO: remove minimum width/height and instead make update_screen()
    // do something sensible when the terminal dimensions are tiny
    term->width = MAX(w, 3);
    term->height = MAX(h, 3);

    update_window_sizes(term, root_frame);
    LOG_INFO("terminal size: %ux%u", w, h);
}

void update_screen(EditorState *e, const ScreenState *s)
{
    BUG_ON(e->flags & EFLAG_HEADLESS);
    Buffer *buffer = e->buffer;
    Terminal *term = &e->terminal;
    ScreenUpdateFlags flags = e->screen_update;
    start_update(term);

    if (flags & UPDATE_ALL_WINDOWS) {
        update_all_windows(e);
    } else {
        View *view = e->view;
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
                buffer_mark_tabbars_changed(buffer);
                flags |= UPDATE_TERM_TITLE;
            }
        } else {
            e->window->update_tabbar = true;
            mark_all_lines_changed(buffer);
            flags |= UPDATE_TERM_TITLE;
        }
        update_buffer_windows(e);
        if (unlikely(flags & UPDATE_WINDOW_SEPARATORS)) {
            update_window_separators(e);
        }
    }

    if (flags & UPDATE_TERM_TITLE) {
        update_term_title(term, buffer, e->options.set_window_title);
    }

    update_command_line(e);

    if (flags & UPDATE_CURSOR_STYLE) {
        update_cursor_style(e);
    }

    if (unlikely(flags & UPDATE_DIALOG)) {
        bool u;
        show_dialog(term, &e->styles, get_msg(&u));
    } else {
        restore_cursor(e);
        term_show_cursor(term);
    }

    e->screen_update = 0;
    end_update(e);
}
