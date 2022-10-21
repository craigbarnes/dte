#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "editor.h"
#include "encoding.h"
#include "frame.h"
#include "options.h"
#include "util/ptr-array.h"
#include "view.h"

enum {
    // Minimum width of line numbers bar (including padding)
    LINE_NUMBERS_MIN_WIDTH = 5
};

Window *new_window(void);
View *window_add_buffer(Window *w, Buffer *b);
View *window_open_empty_buffer(EditorState *e, Window *w);
View *window_open_buffer(EditorState *e, Window *w, const char *filename, bool must_exist, const Encoding *encoding);
View *window_get_view(Window *w, Buffer *b);
View *window_find_view(Window *w, Buffer *b);
View *window_find_unclosable_view(Window *w);
void window_free(EditorState *e, Window *w);
size_t remove_view(EditorState *e, View *v);
void window_close_current(EditorState *e);
void window_close_current_view(EditorState *e, Window *w);
void set_view(EditorState *e, View *v);
View *window_open_new_file(EditorState *e, Window *w);
View *window_open_file(EditorState *e, Window *w, const char *filename, const Encoding *encoding);
void window_open_files(EditorState *e, Window *w, char **filenames, const Encoding *encoding);
void mark_buffer_tabbars_changed(Buffer *b);
void calculate_line_numbers(Window *win, const GlobalOptions *options);
void set_window_coordinates(Window *win, int x, int y, const GlobalOptions *options);
void set_window_size(Window *win, int w, int h, const GlobalOptions *options);
int window_get_scroll_margin(const Window *w, unsigned int scroll_margin);
void frame_for_each_window(const Frame *f, void (*func)(Window*, void*), void *data);
Window *prev_window(EditorState *e, Window *w);
Window *next_window(EditorState *e, Window *w);

#endif
