#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "frame.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "view.h"

enum {
    // Minimum width of line numbers bar (including padding)
    LINE_NUMBERS_MIN_WIDTH = 5
};

// A sub-division of the screen, similar to a window in a tiling window
// manager. There can be multiple Views associated with each Window, but
// only one is visible at a time. Each tab displayed in the tab bar
// corresponds to a View and the editable text area corresponds to the
// Buffer of the *current* View (Window::view::buffer).
typedef struct Window {
    struct EditorState *editor; // Editor session containing this Window
    PointerArray views; // All Views contained by this Window
    Frame *frame; // Parent Frame containing this Window
    View *view; // Current View
    View *prev_view; // Previous view, if set
    int x, y; // Coordinates for top left of window
    int w, h; // Width and height of window (including tabbar and statusline)
    int edit_x, edit_y; // Top left of editable area
    int edit_w, edit_h; // Width and height of editable area
    size_t first_tab_idx;
    bool update_tabbar;
    struct {
        int width;
        long first;
        long last;
    } line_numbers;
} Window;

static inline View *window_get_first_view(const Window *window)
{
    BUG_ON(window->views.count == 0);
    return window->views.ptrs[0];
}

struct EditorState;

Window *new_window(struct EditorState *e) NONNULL_ARGS_AND_RETURN;
View *window_add_buffer(Window *window, Buffer *buffer) NONNULL_ARGS_AND_RETURN;
View *window_open_empty_buffer(Window *window) NONNULL_ARGS_AND_RETURN;
View *window_open_buffer(Window *window, const char *filename, bool must_exist, const char *encoding) NONNULL_ARG(1, 2);
View *window_find_or_create_view(Window *window, Buffer *buffer) NONNULL_ARGS_AND_RETURN;
View *window_find_view(Window *window, Buffer *buffer) NONNULL_ARGS;
size_t window_count_uncloseable_views(const Window *window, View **first_uncloseable) NONNULL_ARGS WRITEONLY(2);
void window_remove_view_at_index(Window *window, size_t view_idx) NONNULL_ARGS;
void window_free(Window *window) NONNULL_ARGS;
void window_close(Window *window) NONNULL_ARGS;
void window_close_current_view(Window *window) NONNULL_ARGS;
View *window_open_new_file(Window *window) NONNULL_ARGS;
View *window_open_file(Window *window, const char *filename, const char *encoding) NONNULL_ARG(1, 2);
View *window_open_files(Window *window, char **filenames, const char *encoding) NONNULL_ARG(1, 2);
void window_calculate_line_numbers(Window *window) NONNULL_ARGS;
void window_set_coordinates(Window *window, int x, int y) NONNULL_ARGS;
void window_set_size(Window *window, int w, int h) NONNULL_ARGS;
unsigned int window_get_scroll_margin(const Window *window) NONNULL_ARGS;
Window *window_prev(Window *window) NONNULL_ARGS_AND_RETURN;
Window *window_next(Window *window) NONNULL_ARGS_AND_RETURN;
void frame_for_each_window(const Frame *frame, void (*func)(Window*, void*), void *data) NONNULL_ARG(1, 2);
void buffer_mark_tabbars_changed(Buffer *buffer) NONNULL_ARGS;
void set_view(View *view) NONNULL_ARGS;

#endif
