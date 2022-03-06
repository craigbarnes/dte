#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "editor.h"
#include "syntax/color.h"
#include "terminal/terminal.h"
#include "view.h"
#include "window.h"

// screen.c
void update_term_title(Terminal *term, const Buffer *b);
void update_separators(Terminal *term);
void update_window_sizes(EditorState *e);
void update_line_numbers(Terminal *term, Window *win, bool force);
void update_screen_size(EditorState *e);
void set_color(Terminal *term, const TermColor *color);
void set_builtin_color(Terminal *term, BuiltinColorEnum c);
void mask_color(TermColor *color, const TermColor *over);

// screen-cmdline.c
void update_command_line(Terminal *term);
void show_message(Terminal *term, const char *msg, bool is_error);

// screen-tabbar.c
void print_tabbar(Terminal *term, Window *w);

// screen-status.c
void update_status_line(Terminal *term, const Window *win);
size_t statusline_format_find_error(const char *str);

// screen-view.c
void update_range(Terminal *term, const View *v, long y1, long y2);

#endif
