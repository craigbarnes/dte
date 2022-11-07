#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "editor.h"
#include "syntax/color.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "view.h"
#include "window.h"

// screen.c
void update_term_title(Terminal *term, const Buffer *b, bool set_window_title);
void update_separators(Terminal *term, const ColorScheme *colors, const Frame *frame);
void update_window_sizes(Terminal *term, Frame *frame);
void update_screen_size(Terminal *term, Frame *root_frame);
void update_line_numbers(Terminal *term, const ColorScheme *colors, Window *win, bool force);
void update_cursor_style(EditorState *e);
void set_color(Terminal *term, const ColorScheme *colors, const TermColor *color);
void set_builtin_color(Terminal *term, const ColorScheme *colors, BuiltinColorEnum c);
void mask_color(TermColor *color, const TermColor *over);

// screen-cmdline.c
void update_command_line(EditorState *e);
void show_message(Terminal *term, const ColorScheme *colors, const char *msg, bool is_error);

// screen-tabbar.c
void print_tabbar(Terminal *term, const ColorScheme *colors, Window *window);

// screen-status.c
void update_status_line(const Window *win);

// screen-view.c
void update_range(EditorState *e, const View *v, long y1, long y2);

#endif
