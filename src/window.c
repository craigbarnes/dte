#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "window.h"
#include "editor.h"
#include "error.h"
#include "file-history.h"
#include "load-save.h"
#include "lock.h"
#include "move.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"

Window *new_window(EditorState *e)
{
    Window *window = xnew0(Window, 1);
    window->editor = e;
    return window;
}

View *window_add_buffer(Window *window, Buffer *buffer)
{
    // We rely on this being 0, for implicit initialization of
    // View::selection and View::select_mode
    static_assert(SELECT_NONE == 0);

    View *view = xnew(View, 1);
    *view = (View) {
        .buffer = buffer,
        .window = window,
        .cursor = {
            .blk = BLOCK(buffer->blocks.next),
            .head = &buffer->blocks,
        }
    };

    ptr_array_append(&buffer->views, view);
    ptr_array_append(&window->views, view);
    window->update_tabbar = true;
    return view;
}

View *window_open_empty_buffer(Window *window)
{
    EditorState *e = window->editor;
    return window_add_buffer(window, open_empty_buffer(&e->buffers, &e->options));
}

View *window_open_buffer (
    Window *window,
    const char *filename,
    bool must_exist,
    const Encoding *encoding
) {
    if (unlikely(filename[0] == '\0')) {
        error_msg("Empty filename not allowed");
        return NULL;
    }

    EditorState *e = window->editor;
    bool dir_missing = false;
    char *absolute = path_absolute(filename);
    if (absolute) {
        // Already open?
        Buffer *buffer = find_buffer(&e->buffers, absolute);
        if (buffer) {
            if (!streq(absolute, buffer->abs_filename)) {
                const char *bufname = buffer_filename(buffer);
                char *s = short_filename(absolute, &e->home_dir);
                info_msg("%s and %s are the same file", s, bufname);
                free(s);
            }
            free(absolute);
            return window_get_view(window, buffer);
        }
    } else {
        // Let load_buffer() create error message
        dir_missing = (errno == ENOENT);
    }

    /*
    /proc/$PID/fd/ contains symbolic links to files that have been opened
    by process $PID. Some of the files may have been deleted but can still
    be opened using the symbolic link but not by using the absolute path.

    # create file
    mkdir /tmp/x
    echo foo > /tmp/x/file

    # in another shell: keep the file open
    tail -f /tmp/x/file

    # make the absolute path unavailable
    rm /tmp/x/file
    rmdir /tmp/x

    # this should still succeed
    dte /proc/$(pidof tail)/fd/3
    */

    Buffer *buffer = buffer_new(&e->buffers, &e->options, encoding);
    if (!load_buffer(buffer, filename, &e->options, must_exist)) {
        remove_and_free_buffer(&e->buffers, buffer);
        free(absolute);
        return NULL;
    }
    if (unlikely(buffer->file.mode == 0 && dir_missing)) {
        // New file in non-existing directory; this is usually a mistake
        error_msg("Error opening %s: Directory does not exist", filename);
        remove_and_free_buffer(&e->buffers, buffer);
        free(absolute);
        return NULL;
    }

    if (absolute) {
        buffer->abs_filename = absolute;
    } else {
        // FIXME: obviously wrong
        buffer->abs_filename = xstrdup(filename);
    }
    update_short_filename(buffer, &e->home_dir);

    if (e->options.lock_files) {
        if (!lock_file(buffer->abs_filename)) {
            buffer->readonly = true;
        } else {
            buffer->locked = true;
        }
    }

    if (buffer->file.mode != 0 && !buffer->readonly && access(filename, W_OK)) {
        error_msg("No write permission to %s, marking read-only", filename);
        buffer->readonly = true;
    }

    return window_add_buffer(window, buffer);
}

View *window_get_view(Window *window, Buffer *buffer)
{
    View *view = window_find_view(window, buffer);
    if (!view) {
        // Open the buffer in other window to this window
        view = window_add_buffer(window, buffer);
        view->cursor = ((View*)buffer->views.ptrs[0])->cursor;
    }
    return view;
}

View *window_find_view(Window *window, Buffer *buffer)
{
    for (size_t i = 0, n = buffer->views.count; i < n; i++) {
        View *view = buffer->views.ptrs[i];
        if (view->window == window) {
            return view;
        }
    }
    // Buffer isn't open in this window
    return NULL;
}

View *window_find_unclosable_view(Window *window)
{
    // Check active view first
    if (window->view && !view_can_close(window->view)) {
        return window->view;
    }
    for (size_t i = 0, n = window->views.count; i < n; i++) {
        View *view = window->views.ptrs[i];
        if (!view_can_close(view)) {
            return view;
        }
    }
    return NULL;
}

static void window_remove_views(Window *window)
{
    while (window->views.count > 0) {
        View *view = window->views.ptrs[window->views.count - 1];
        remove_view(view);
    }
}

// NOTE: window->frame isn't removed
void window_free(Window *window)
{
    window_remove_views(window);
    free(window->views.ptrs);
    window->frame = NULL;
    free(window);
}

// Remove view from view->window and view->buffer->views and free it
size_t remove_view(View *view)
{
    Window *window = view->window;
    EditorState *e = window->editor;
    if (view == window->prev_view) {
        window->prev_view = NULL;
    }
    if (view == e->view) {
        e->view = NULL;
        e->buffer = NULL;
    }

    size_t idx = ptr_array_idx(&window->views, view);
    BUG_ON(idx >= window->views.count);
    ptr_array_remove_idx(&window->views, idx);
    window->update_tabbar = true;

    Buffer *buffer = view->buffer;
    ptr_array_remove(&buffer->views, view);
    if (buffer->views.count == 0) {
        if (buffer->options.file_history && buffer->abs_filename) {
            FileHistory *hist = &e->file_history;
            file_history_add(hist, view->cy + 1, view->cx_char + 1, buffer->abs_filename);
        }
        remove_and_free_buffer(&e->buffers, buffer);
    }

    free(view);
    return idx;
}

void window_close_current_view(Window *window)
{
    size_t idx = remove_view(window->view);
    if (window->prev_view) {
        window->view = window->prev_view;
        window->prev_view = NULL;
        return;
    }
    if (window->views.count == 0) {
        window_open_empty_buffer(window);
    }
    if (window->views.count == idx) {
        idx--;
    }
    window->view = window->views.ptrs[idx];
}

static void restore_cursor_from_history(const FileHistory *hist, View *view)
{
    unsigned long row, col;
    if (file_history_find(hist, view->buffer->abs_filename, &row, &col)) {
        move_to_filepos(view, row, col);
    }
}

void set_view(View *view)
{
    EditorState *e = view->window->editor;
    if (e->view == view) {
        return;
    }

    // Forget previous view when changing view using any other command but open
    if (e->window) {
        e->window->prev_view = NULL;
    }

    e->view = view;
    e->buffer = view->buffer;
    e->window = view->window;
    e->window->view = view;

    if (!view->buffer->setup) {
        buffer_setup(e, view->buffer);
        if (view->buffer->options.file_history && view->buffer->abs_filename) {
            restore_cursor_from_history(&e->file_history, view);
        }
    }

    // view.cursor can be invalid if same buffer was modified from another view
    if (view->restore_cursor) {
        view->cursor.blk = BLOCK(view->buffer->blocks.next);
        block_iter_goto_offset(&view->cursor, view->saved_cursor_offset);
        view->restore_cursor = false;
        view->saved_cursor_offset = 0;
    }

    // Save cursor states of views sharing same buffer
    for (size_t i = 0, n = view->buffer->views.count; i < n; i++) {
        View *other = view->buffer->views.ptrs[i];
        if (other != view) {
            other->saved_cursor_offset = block_iter_get_offset(&other->cursor);
            other->restore_cursor = true;
        }
    }
}

View *window_open_new_file(Window *window)
{
    View *prev = window->view;
    View *view = window_open_empty_buffer(window);
    set_view(view);
    window->prev_view = prev;
    return view;
}

static bool buffer_is_empty_and_untouched(const Buffer *b)
{
    return !b->abs_filename && b->change_head.nr_prev == 0 && !b->display_filename;
}

// If window contains only one untouched buffer it'll be closed after
// opening another file. This is done because closing the last buffer
// causes an empty buffer to be opened (windows must contain at least
// one buffer).
static bool is_useless_empty_view(const View *v)
{
    return v && v->window->views.count == 1 && buffer_is_empty_and_untouched(v->buffer);
}

View *window_open_file(Window *window, const char *filename, const Encoding *encoding)
{
    View *prev = window->view;
    bool useless = is_useless_empty_view(prev);
    View *view = window_open_buffer(window, filename, false, encoding);
    if (view) {
        set_view(view);
        if (useless) {
            remove_view(prev);
        } else {
            window->prev_view = prev;
        }
    }
    return view;
}

// Open multiple files in window and return the first opened View
View *window_open_files(Window *window, char **filenames, const Encoding *encoding)
{
    View *empty = window->view;
    bool useless = is_useless_empty_view(empty);
    View *first = NULL;

    for (size_t i = 0; filenames[i]; i++) {
        View *view = window_open_buffer(window, filenames[i], false, encoding);
        if (view && !first) {
            set_view(view);
            first = view;
        }
    }

    if (useless && window->view != empty) {
        remove_view(empty);
    }

    return first;
}

void mark_buffer_tabbars_changed(Buffer *buffer)
{
    for (size_t i = 0, n = buffer->views.count; i < n; i++) {
        View *view = buffer->views.ptrs[i];
        view->window->update_tabbar = true;
    }
}

static int line_numbers_width(const Window *window, const GlobalOptions *options)
{
    if (!options->show_line_numbers || !window->view) {
        return 0;
    }
    size_t width = size_str_width(window->view->buffer->nl) + 1;
    return MAX(width, LINE_NUMBERS_MIN_WIDTH);
}

static int edit_x_offset(const Window *window, const GlobalOptions *options)
{
    return line_numbers_width(window, options);
}

static int edit_y_offset(const GlobalOptions *options)
{
    return options->tab_bar ? 1 : 0;
}

static void set_edit_size(Window *window, const GlobalOptions *options)
{
    int xo = edit_x_offset(window, options);
    int yo = edit_y_offset(options);

    window->edit_w = window->w - xo;
    window->edit_h = window->h - yo - 1; // statusline
    window->edit_x = window->x + xo;
}

void calculate_line_numbers(Window *window)
{
    const GlobalOptions *options = &window->editor->options;
    int w = line_numbers_width(window, options);
    if (w != window->line_numbers.width) {
        window->line_numbers.width = w;
        window->line_numbers.first = 0;
        window->line_numbers.last = 0;
        mark_all_lines_changed(window->view->buffer);
    }
    set_edit_size(window, options);
}

void set_window_coordinates(Window *window, int x, int y)
{
    const GlobalOptions *options = &window->editor->options;
    window->x = x;
    window->y = y;
    window->edit_x = x + edit_x_offset(window, options);
    window->edit_y = y + edit_y_offset(options);
}

void set_window_size(Window *window, int w, int h)
{
    window->w = w;
    window->h = h;
    calculate_line_numbers(window);
}

int window_get_scroll_margin(const Window *window, unsigned int scroll_margin)
{
    int max = (window->edit_h - 1) / 2;
    BUG_ON(max < 0);
    return MIN(max, scroll_margin);
}

void frame_for_each_window(const Frame *frame, void (*func)(Window*, void*), void *data)
{
    if (frame->window) {
        func(frame->window, data);
        return;
    }
    for (size_t i = 0, n = frame->frames.count; i < n; i++) {
        frame_for_each_window(frame->frames.ptrs[i], func, data);
    }
}

typedef struct {
    const Window *const target; // Window to search for (set at init.)
    Window *first; // Window passed in first callback invocation
    Window *last; // Window passed in last callback invocation
    Window *prev; // Window immediately before target (if any)
    Window *next; // Window immediately after target (if any)
    bool found; // Set to true when target is found
} WindowCallbackData;

static void find_prev_and_next(Window *window, void *ud)
{
    WindowCallbackData *data = ud;
    data->last = window;
    if (data->found) {
        if (!data->next) {
            data->next = window;
        }
        return;
    }
    if (!data->first) {
        data->first = window;
    }
    if (window == data->target) {
        data->found = true;
        return;
    }
    data->prev = window;
}

Window *prev_window(Window *window)
{
    WindowCallbackData data = {.target = window};
    frame_for_each_window(window->editor->root_frame, find_prev_and_next, &data);
    BUG_ON(!data.found);
    return data.prev ? data.prev : data.last;
}

Window *next_window(Window *window)
{
    WindowCallbackData data = {.target = window};
    frame_for_each_window(window->editor->root_frame, find_prev_and_next, &data);
    BUG_ON(!data.found);
    return data.next ? data.next : data.first;
}

void window_close(Window *window)
{
    EditorState *e = window->editor;
    if (!window->frame->parent) {
        // Don't close last window
        window_remove_views(window);
        set_view(window_open_empty_buffer(window));
        return;
    }

    WindowCallbackData data = {.target = window};
    frame_for_each_window(e->root_frame, find_prev_and_next, &data);
    BUG_ON(!data.found);
    Window *next_or_prev = data.next ? data.next : data.prev;
    BUG_ON(!next_or_prev);

    remove_frame(e, window->frame);
    e->window = NULL;
    set_view(next_or_prev->view);

    mark_everything_changed(e);
    debug_frame(e->root_frame);
}
