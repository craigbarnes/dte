#include "screen.h"

static void print_separator(Window *window, void *ud)
{
    Terminal *term = ud;
    TermOutputBuffer *obuf = &term->obuf;
    if (window->x + window->w == term->width) {
        return;
    }
    for (int y = 0, h = window->h; y < h; y++) {
        term_move_cursor(obuf, window->x + window->w, window->y + y);
        term_add_byte(obuf, '|');
    }
}

static void update_separators(Terminal *term, const ColorScheme *colors, const Frame *frame)
{
    set_builtin_color(term, colors, BC_STATUSLINE);
    frame_for_each_window(frame, print_separator, term);
}

static void update_line_numbers(Terminal *term, const ColorScheme *colors, Window *window, bool force)
{
    const View *view = window->view;
    size_t lines = view->buffer->nl;
    int x = window->x;

    calculate_line_numbers(window);
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
    set_builtin_color(term, colors, BC_LINENUMBER);

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
        term_add_bytes(obuf, buf, width);
    }
}

static void update_window_full(Window *window, void* UNUSED_ARG(data))
{
    EditorState *e = window->editor;
    View *view = window->view;
    view_update_cursor_x(view);
    view_update_cursor_y(view);
    view_update(view, e->options.scroll_margin);
    if (e->options.tab_bar) {
        print_tabbar(&e->terminal, &e->colors, window);
    }
    if (e->options.show_line_numbers) {
        update_line_numbers(&e->terminal, &e->colors, window, true);
    }
    update_range(e, view, view->vy, view->vy + window->edit_h);
    update_status_line(window);
}

void update_all_windows(EditorState *e)
{
    update_window_sizes(&e->terminal, e->root_frame);
    frame_for_each_window(e->root_frame, update_window_full, NULL);
    update_separators(&e->terminal, &e->colors, e->root_frame);
}

static void update_window(EditorState *e, Window *window)
{
    if (e->options.tab_bar && window->update_tabbar) {
        print_tabbar(&e->terminal, &e->colors, window);
    }

    const View *view = window->view;
    if (e->options.show_line_numbers) {
        // Force updating lines numbers if all lines changed
        bool force = (view->buffer->changed_line_max == LONG_MAX);
        update_line_numbers(&e->terminal, &e->colors, window, force);
    }

    long y1 = MAX(view->buffer->changed_line_min, view->vy);
    long y2 = MIN(view->buffer->changed_line_max, view->vy + window->edit_h - 1);
    update_range(e, view, y1, y2 + 1);
    update_status_line(window);
}

// Update all visible views containing this buffer
void update_buffer_windows(EditorState *e, const Buffer *buffer)
{
    const View *current_view = e->view;
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

            // These have already been updated for current view
            view_update_cursor_x(view);
            view_update_cursor_y(view);
            view_update(view, e->options.scroll_margin);
        }
        update_window(e, view->window);
    }
}
