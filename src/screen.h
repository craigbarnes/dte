#ifndef SCREEN_H
#define SCREEN_H

#include "buffer.h"
#include "color.h"
#include "term.h"
#include "view.h"
#include "window.h"

void set_color(TermColor *color);
void set_builtin_color(enum builtin_color c);
void mask_color(TermColor *color, const TermColor *over);

void print_tabbar(Window *w);
int print_command(char prefix);
void print_message(const char *msg, bool is_error);
void save_term_title(void);
void restore_term_title(void);
void update_term_title(Buffer *b);
void update_range(View *v, int y1, int y2);
void update_separators(void);
void update_status_line(Window *win);
void update_window_sizes(void);
void update_line_numbers(Window *win, bool force);
void update_screen_size(void);

#endif
