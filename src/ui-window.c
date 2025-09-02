#include "ui.h"
#include "editor.h"

static void print_separator(Window *window, void* UNUSED_ARG(ud))
{
    EditorState *e = window->editor;
    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;
    unsigned int x = window->x + window->w;
    if (x >= term->width) {
        // Window is on right edge; no separator needed
        return;
    }

    const char sep = (e->options.window_separator == WINSEP_BAR) ? '|' : ' ';
    for (unsigned int y = window->y, n = y + window->h; y < n; y++) {
        term_move_cursor(obuf, x, y);
        term_put_byte(obuf, sep);
    }
}

void update_window_separators(Terminal *term, Frame *root_frame, const StyleMap *styles)
{
    if (root_frame->window) {
        // Only 1 window in use; no separators needed
        return;
    }

    set_builtin_style(term, styles, BSE_STATUSLINE);
    frame_for_each_window(root_frame, print_separator, NULL);
}

static void update_line_numbers(Terminal *term, const StyleMap *styles, Window *window, bool force)
{
    const View *view = window->view;
    size_t lines = view->buffer->nl;
    int x = window->x;

    window_calculate_line_numbers(window);
    long first = view->vy + 1;
    long last = MIN(view->vy + window->edit_h, lines);

    if (
        !force
        && window->line_numbers.first == first
        && window->line_numbers.last == last
    ) {
        return;
    }

    window->line_numbers.first = first;
    window->line_numbers.last = last;

    TermOutputBuffer *obuf = &term->obuf;
    char buf[DECIMAL_STR_MAX(unsigned long) + 1];
    size_t width = window->line_numbers.width;
    BUG_ON(width > sizeof(buf));
    BUG_ON(width < LINE_NUMBERS_MIN_WIDTH);
    term_output_reset(term, window->x, window->w, 0);
    set_builtin_style(term, styles, BSE_LINENUMBER);

    for (int y = 0, h = window->edit_h, edit_y = window->edit_y; y < h; y++) {
        unsigned long line = view->vy + y + 1;
        memset(buf, ' ', width);
        if (line <= lines) {
            size_t i = width - 2;
            do {
                buf[i--] = (line % 10) + '0';
            } while (line /= 10);
        }
        term_move_cursor(obuf, x, edit_y + y);
        term_put_bytes(obuf, buf, width);
    }
}

static void update_window_full(Window *window, void* UNUSED_ARG(data))
{
    EditorState *e = window->editor;
    View *view = window->view;
    const GlobalOptions *options = &e->options;
    const StyleMap *styles = &e->styles;
    Terminal *term = &e->terminal;
    view_update(view);

    if (options->tab_bar) {
        print_tabbar(term, styles, window);
    }

    if (options->show_line_numbers) {
        update_line_numbers(term, styles, window, true);
    }

    bool display_special = options->display_special;
    long y2 = view->vy + window->edit_h;
    update_range(term, view, styles, view->vy, y2, display_special);
    update_status_line(window);
}

void update_all_windows(Terminal *term, Frame *root_frame, const StyleMap *styles)
{
    update_window_sizes(term, root_frame);
    frame_for_each_window(root_frame, update_window_full, NULL);
    update_window_separators(term, root_frame, styles);
}

static void update_window (
    Terminal *term,
    Window *window,
    const StyleMap *styles,
    const GlobalOptions *options
) {
    if (options->tab_bar && window->update_tabbar) {
        print_tabbar(term, styles, window);
    }

    const View *view = window->view;
    const Buffer *buffer = view->buffer;
    if (options->show_line_numbers) {
        // Force updating line numbers if all lines changed
        bool force = (buffer->changed_line_max == LONG_MAX);
        update_line_numbers(term, styles, window, force);
    }

    long y1 = MAX(buffer->changed_line_min, view->vy);
    long y2 = MIN(buffer->changed_line_max, view->vy + window->edit_h - 1);
    update_range(term, view, styles, y1, y2 + 1, options->display_special);
    update_status_line(window);
}

// Update all visible views containing the current buffer
void update_buffer_windows (
    Terminal *term,
    const View *current_view,
    const StyleMap *styles,
    const GlobalOptions *options
) {
    const Buffer *buffer = current_view->buffer;
    for (size_t i = 0, n = buffer->views.count; i < n; i++) {
        View *view = buffer->views.ptrs[i];
        if (view != view->window->view) {
            // Not visible
            continue;
        }
        if (view != current_view) {
            // Restore cursor
            view->cursor.blk = BLOCK(view->buffer->blocks.next);
            block_iter_goto_offset(&view->cursor, view->saved_cursor_offset);

            // This has already been done for the current view
            view_update(view);
        }
        update_window(term, view->window, styles, options);
    }
}
