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

static void update_cursor_style (
    Terminal *term,
    const Buffer *buffer,
    const TermCursorStyle *cursor_styles,
    bool is_normal_mode
) {
    CursorInputMode mode;
    if (is_normal_mode) {
        bool overwrite = buffer->options.overwrite;
        mode = overwrite ? CURSOR_MODE_OVERWRITE : CURSOR_MODE_INSERT;
    } else {
        mode = CURSOR_MODE_CMDLINE;
    }

    TermCursorStyle style = cursor_styles[mode];
    TermCursorStyle def = cursor_styles[CURSOR_MODE_DEFAULT];
    if (style.type == CURSOR_KEEP) {
        style.type = def.type;
    }
    if (style.color == COLOR_KEEP) {
        style.color = def.color;
    }

    if (!same_cursor(&style, &term->obuf.cursor_style)) {
        term_set_cursor_style(term, style);
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

static void restore_cursor (
    Terminal *term,
    const View *view,
    bool is_normal_mode,
    size_t cmdline_x
) {
    unsigned int x, y;
    if (is_normal_mode) {
        const Window *window = view->window;
        x = window->edit_x + view->cx_display - view->vx;
        y = window->edit_y + view->cy - view->vy;
    } else {
        x = cmdline_x;
        y = term->height - 1;
    }
    term_move_cursor(&term->obuf, x, y);
}

static void clear_update_tabbar(Window *window, void* UNUSED_ARG(data))
{
    window->update_tabbar = false;
}

static void end_update(Terminal *term, Buffer *buffer, const Frame *root_frame)
{
    term_end_sync_update(term);
    term_output_flush(&term->obuf);

    buffer->changed_line_min = LONG_MAX;
    buffer->changed_line_max = -1;

    if (root_frame->window) {
        // Only 1 window to update
        clear_update_tabbar(root_frame->window, NULL);
    } else {
        frame_for_each_window(root_frame, clear_update_tabbar, NULL);
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
    const GlobalOptions *options = &e->options;
    const StyleMap *styles = &e->styles;
    Frame *root_frame = e->root_frame;
    View *view = e->view;
    Buffer *buffer = e->buffer;
    Terminal *term = &e->terminal;
    ScreenUpdateFlags flags = e->screen_update;
    start_update(term);

    if (flags & UPDATE_ALL_WINDOWS) {
        update_all_windows(term, root_frame, styles);
    } else {
        view_update(view, options->scroll_margin);
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
        update_buffer_windows(term, view, styles, options);
        if (unlikely(flags & UPDATE_WINDOW_SEPARATORS)) {
            update_window_separators(term, root_frame, styles);
        }
    }

    if (flags & UPDATE_TERM_TITLE) {
        update_term_title(term, buffer, options->set_window_title);
    }

    bool is_normal_mode = (e->mode->cmds == &normal_commands);
    size_t cmdline_x = update_command_line(term, styles, &e->cmdline, &e->search, e->mode);

    if (flags & UPDATE_CURSOR_STYLE) {
        update_cursor_style(term, buffer, e->cursor_styles, is_normal_mode);
    }

    if (unlikely(flags & UPDATE_DIALOG)) {
        bool u;
        show_dialog(term, styles, get_msg(&u));
    } else {
        restore_cursor(term, view, is_normal_mode, cmdline_x);
        term_show_cursor(term);
    }

    e->screen_update = 0;
    end_update(term, buffer, root_frame);
}
