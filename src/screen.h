#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "terminal/color.h"
#include "view.h"
#include "window.h"

void set_color(const TermColor *color);
void set_builtin_color(enum builtin_color c);
void mask_color(TermColor *color, const TermColor *over);

void print_tabbar(Window *w);
size_t print_command(char prefix);
void show_message(const char *msg, bool is_error);
void update_command_line(void);
void update_term_title(const Buffer *b);
void update_range(const View *v, long y1, long y2);
void update_separators(void);
void update_status_line(const Window *win);
void update_window_sizes(void);
void update_line_numbers(Window *win, bool force);
void update_screen_size(void);

#endif
