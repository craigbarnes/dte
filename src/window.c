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
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xstring.h"

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
        .cursor = block_iter(buffer),
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
    const char *encoding
) {
    ErrorBuffer *ebuf = window->editor->err;
    if (unlikely(filename[0] == '\0')) {
        error_msg(ebuf, "Empty filename not allowed");
        return NULL;
    }

    char *absolute = path_absolute(filename);
    if (!absolute) {
        bool nodir = (errno == ENOENT); // New file in non-existing dir (usually a mistake)
        const char *err = nodir ? "Directory does not exist" : strerror(errno);
        error_msg(ebuf, "Error opening %s: %s", filename, err);
        return NULL;
    }

    EditorState *e = window->editor;
    Buffer *buffer = find_buffer(&e->buffers, absolute);
    if (buffer) {
        // File already open in editor
        if (!streq(absolute, buffer->abs_filename)) {
            const char *bufname = buffer_filename(buffer);
            char *s = short_filename(absolute, &e->home_dir);
            info_msg(ebuf, "%s and %s are the same file", s, bufname); // Hard links
            free(s);
        }
        free(absolute);
        return window_find_or_create_view(window, buffer);
    }

    buffer = buffer_new(&e->buffers, &e->options, encoding);
    if (!load_buffer(buffer, filename, &e->options, must_exist)) {
        buffer_remove_unlock_and_free(&e->buffers, buffer, &e->locks_ctx);
        free(absolute);
        return NULL;
    }

    BUG_ON(!absolute);
    BUG_ON(!path_is_absolute(absolute));
    buffer->abs_filename = absolute;
    buffer_update_short_filename(buffer, &e->home_dir);

    if (e->options.lock_files) {
        if (!lock_file(&e->locks_ctx, buffer->abs_filename)) {
            buffer->readonly = true;
        } else {
            buffer->locked = true;
        }
    }

    if (buffer->file.mode != 0 && !buffer->readonly && access(filename, W_OK)) {
        error_msg(ebuf, "No write permission to %s, marking read-only", filename);
        buffer->readonly = true;
    }

    return window_add_buffer(window, buffer);
}

View *window_find_or_create_view(Window *window, Buffer *buffer)
{
    View *view = window_find_view(window, buffer);
    if (!view) {
        // Buffer isn't open in this window; create a new view of it
        view = window_add_buffer(window, buffer);
        view->cursor = buffer_get_first_view(buffer)->cursor;
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

    void **ptrs = window->views.ptrs;
    for (size_t i = 0, n = window->views.count; i < n; i++) {
        View *view = ptrs[i];
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
    ptr_array_free_array(&window->views);
    window->frame = NULL;
    free(window);
}

// Remove view from view->window and view->buffer->views and free it
size_t remove_view(View *view)
{
    Window *window = view->window;
    window->update_tabbar = true;
    if (view == window->prev_view) {
        window->prev_view = NULL;
    }

    EditorState *e = window->editor;
    if (view == e->view) {
        e->view = NULL;
        e->buffer = NULL;
    }

    size_t idx = ptr_array_remove(&window->views, view);
    Buffer *buffer = view->buffer;
    ptr_array_remove(&buffer->views, view);

    if (buffer->views.count == 0) {
        if (buffer->options.file_history && buffer->abs_filename) {
            FileHistory *hist = &e->file_history;
            file_history_append(hist, view->cy + 1, view->cx_char + 1, buffer->abs_filename);
        }
        buffer_remove_unlock_and_free(&e->buffers, buffer, &e->locks_ctx);
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

    const PointerArray *views = &window->views;
    if (views->count == 0) {
        window_open_empty_buffer(window);
    }

    idx -= (views->count == idx);
    window->view = views->ptrs[idx];
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
    void **ptrs = view->buffer->views.ptrs;
    for (size_t i = 0, n = view->buffer->views.count; i < n; i++) {
        View *other = ptrs[i];
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

View *window_open_file(Window *window, const char *filename, const char *encoding)
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
View *window_open_files(Window *window, char **filenames, const char *encoding)
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

void buffer_mark_tabbars_changed(Buffer *buffer)
{
    for (size_t i = 0, n = buffer->views.count; i < n; i++) {
        View *view = buffer->views.ptrs[i];
        view->window->update_tabbar = true;
    }
}

static int line_numbers_width(const GlobalOptions *options, const View *view)
{
    if (!options->show_line_numbers || !view) {
        return 0;
    }
    size_t width = size_str_width(view->buffer->nl) + 1;
    return MAX(width, LINE_NUMBERS_MIN_WIDTH);
}

static int edit_x_offset(const GlobalOptions *options, const View *view)
{
    return line_numbers_width(options, view);
}

static int edit_y_offset(const GlobalOptions *options)
{
    return options->tab_bar ? 1 : 0;
}

static void set_edit_size(Window *window, const GlobalOptions *options)
{
    int xo = edit_x_offset(options, window->view);
    int yo = edit_y_offset(options);
    window->edit_w = window->w - xo;
    window->edit_h = window->h - yo - 1; // statusline
    window->edit_x = window->x + xo;
}

void window_calculate_line_numbers(Window *window)
{
    const GlobalOptions *options = &window->editor->options;
    int w = line_numbers_width(options, window->view);
    if (w != window->line_numbers.width) {
        window->line_numbers.width = w;
        window->line_numbers.first = 0;
        window->line_numbers.last = 0;
        mark_all_lines_changed(window->view->buffer);
    }
    set_edit_size(window, options);
}

void window_set_coordinates(Window *window, int x, int y)
{
    const GlobalOptions *options = &window->editor->options;
    window->x = x;
    window->y = y;
    window->edit_x = x + edit_x_offset(options, window->view);
    window->edit_y = y + edit_y_offset(options);
}

void window_set_size(Window *window, int w, int h)
{
    window->w = w;
    window->h = h;
    window_calculate_line_numbers(window);
}

unsigned int window_get_scroll_margin(const Window *window)
{
    int edit_h = window->edit_h;
    if (unlikely(edit_h <= 2)) {
        return 0;
    }

    unsigned int max = (edit_h - 1) / 2;
    unsigned int scroll_margin = window->editor->options.scroll_margin;
    return MIN(max, scroll_margin);
}

// NOLINTNEXTLINE(misc-no-recursion)
void frame_for_each_window(const Frame *frame, void (*func)(Window*, void*), void *data)
{
    if (frame->window) {
        func(frame->window, data);
        return;
    }

    void **ptrs = frame->frames.ptrs;
    for (size_t i = 0, n = frame->frames.count; i < n; i++) {
        frame_for_each_window(ptrs[i], func, data);
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

Window *window_prev(Window *window)
{
    WindowCallbackData data = {.target = window};
    frame_for_each_window(window->editor->root_frame, find_prev_and_next, &data);
    BUG_ON(!data.found);
    return data.prev ? data.prev : data.last;
}

Window *window_next(Window *window)
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

    frame_remove(e, window->frame);
    e->window = NULL;
    set_view(next_or_prev->view);

    e->screen_update |= UPDATE_ALL;
    frame_debug(e->root_frame);
}
