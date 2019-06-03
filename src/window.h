#ifndef WINDOW_H
#define WINDOW_H

#include "buffer.h"
#include "frame.h"
#include "view.h"

typedef struct Window {
    PointerArray views;
    Frame *frame;

    // Current view
    View *view;

    // Previous view if set
    View *prev_view;

    // Coordinates and size of entire window including tabbar and status line
    int x, y;
    int w, h;

    // Coordinates and size of editable area
    int edit_x, edit_y;
    int edit_w, edit_h;

    struct {
        int width;
        long first;
        long last;
    } line_numbers;

    size_t first_tab_idx;

    bool update_tabbar;
} Window;

extern Window *window;

Window *new_window(void);
View *window_add_buffer(Window *w, Buffer *b);
View *window_open_empty_buffer(Window *w);
View *window_open_buffer(Window *w, const char *filename, bool must_exist, const Encoding *encoding);
View *window_get_view(Window *w, Buffer *b);
View *window_find_view(Window *w, Buffer *b);
View *window_find_unclosable_view(Window *w, bool (*can_close)(const View *));
void window_free(Window *w);
void remove_view(View *v);
void window_close_current(void);
void window_close_current_view(Window *w);
void set_view(View *v);
View *window_open_new_file(Window *w);
View *window_open_file(Window *w, const char *filename, const Encoding *encoding);
void window_open_files(Window *w, char **filenames, const Encoding *encoding);
void mark_buffer_tabbars_changed(Buffer *b);
TabBarMode tabbar_visibility(const Window *win);
int vertical_tabbar_width(const Window *win);
void calculate_line_numbers(Window *win);
void set_window_coordinates(Window *win, int x, int y);
void set_window_size(Window *win, int w, int h);
int window_get_scroll_margin(const Window *w);
void for_each_window(void (*func)(Window *w));
Window *prev_window(Window *w);
Window *next_window(Window *w);

#endif
