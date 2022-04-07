#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "editor.h"
#include "syntax/color.h"
#include "terminal/output.h"
#include "view.h"
#include "window.h"

// screen.c
void update_term_title(EditorState *e, const Buffer *b);
void update_separators(EditorState *e);
void update_window_sizes(EditorState *e);
void update_line_numbers(EditorState *e, Window *win, bool force);
void update_screen_size(EditorState *e);
void set_color(EditorState *e, const TermColor *color);
void update_cursor_style(EditorState *e);
void set_builtin_color(EditorState *e, BuiltinColorEnum c);
void mask_color(TermColor *color, const TermColor *over);

// screen-cmdline.c
void update_command_line(EditorState *e);
void show_message(EditorState *e, const char *msg, bool is_error);

// screen-tabbar.c
void print_tabbar(EditorState *e, Window *w);

// screen-status.c
void update_status_line(EditorState *e, const Window *win);
size_t statusline_format_find_error(const char *str);

// screen-view.c
void update_range(EditorState *e, const View *v, long y1, long y2);

#endif
