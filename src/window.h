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
    struct EditorState *editor;
    PointerArray views; // All Views contained by this Window
    Frame *frame;
    View *view; // Current View
    View *prev_view; // Previous view, if set
    int x, y; // Coordinates for top left of window
    int w, h; // Width and height of window (including tabbar and status)
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
View *window_open_buffer(Window *window, const char *filename, bool must_exist, const char *encoding);
View *window_get_view(Window *window, Buffer *buffer);
View *window_find_view(Window *window, Buffer *buffer);
View *window_find_unclosable_view(Window *window);
void window_free(Window *window);
void window_close(Window *window);
void window_close_current_view(Window *window);
View *window_open_new_file(Window *window);
View *window_open_file(Window *window, const char *filename, const char *encoding);
View *window_open_files(Window *window, char **filenames, const char *encoding);
void window_calculate_line_numbers(Window *window);
void window_set_coordinates(Window *window, int x, int y);
void window_set_size(Window *window, int w, int h);
int window_get_scroll_margin(const Window *window, unsigned int scroll_margin);
Window *window_prev(Window *window);
Window *window_next(Window *window);
void frame_for_each_window(const Frame *frame, void (*func)(Window*, void*), void *data);
void buffer_mark_tabbars_changed(Buffer *buffer);
void set_view(View *view);
size_t remove_view(View *view);

#endif
