#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "syntax/color.h"
#include "terminal/output.h"
#include "view.h"
#include "window.h"

// screen.c
void update_term_title(TermOutputBuffer *obuf, const Buffer *b);
void update_separators(TermOutputBuffer *obuf);
void update_window_sizes(void);
void update_line_numbers(TermOutputBuffer *obuf, Window *win, bool force);
void update_screen_size(void);
void set_color(TermOutputBuffer *obuf, const TermColor *color);
void set_builtin_color(TermOutputBuffer *obuf, BuiltinColorEnum c);
void mask_color(TermColor *color, const TermColor *over);

// screen-cmdline.c
void update_command_line(TermOutputBuffer *obuf);
void show_message(TermOutputBuffer *obuf, const char *msg, bool is_error);

// screen-tabbar.c
void print_tabbar(TermOutputBuffer *obuf, Window *w);

// screen-status.c
void update_status_line(TermOutputBuffer *obuf, const Window *win);
size_t statusline_format_find_error(const char *str);

// screen-view.c
void update_range(TermOutputBuffer *obuf, const View *v, long y1, long y2);

#endif
