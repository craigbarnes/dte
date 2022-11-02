#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "encoding.h"
#include "frame.h"
#include "util/ptr-array.h"
#include "view.h"

enum {
    // Minimum width of line numbers bar (including padding)
    LINE_NUMBERS_MIN_WIDTH = 5
};

typedef struct Window {
    struct EditorState *editor;
    PointerArray views;
    Frame *frame;
    View *view; // Current view
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

struct EditorState;

Window *new_window(struct EditorState *e) NONNULL_ARGS_AND_RETURN;
View *window_add_buffer(Window *w, Buffer *b);
View *window_open_empty_buffer(Window *w);
View *window_open_buffer(Window *w, const char *filename, bool must_exist, const Encoding *encoding);
View *window_get_view(Window *w, Buffer *b);
View *window_find_view(Window *w, Buffer *b);
View *window_find_unclosable_view(Window *w);
void window_free(Window *w);
size_t remove_view(View *v);
void window_close_current(struct EditorState *e);
void window_close_current_view(Window *w);
void set_view(struct EditorState *e, View *v);
View *window_open_new_file(Window *w);
View *window_open_file(Window *w, const char *filename, const Encoding *encoding);
void window_open_files(Window *w, char **filenames, const Encoding *encoding);
void mark_buffer_tabbars_changed(Buffer *b);
void calculate_line_numbers(Window *win);
void set_window_coordinates(Window *win, int x, int y);
void set_window_size(Window *win, int w, int h);
int window_get_scroll_margin(const Window *w, unsigned int scroll_margin);
void frame_for_each_window(const Frame *f, void (*func)(Window*, void*), void *data);
Window *prev_window(Window *w);
Window *next_window(Window *w);

#endif
