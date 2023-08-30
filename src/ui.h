#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "editor.h"
#include "syntax/color.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/utf8.h"
#include "view.h"
#include "window.h"

typedef struct {
    bool is_modified;
    unsigned long id;
    long cy;
    long vx;
    long vy;
} ScreenState;

// ui.c
void update_screen(EditorState *e, const ScreenState *s);
void update_term_title(Terminal *term, const Buffer *buffer, bool set_window_title);
void update_window_sizes(Terminal *term, Frame *frame);
void update_screen_size(Terminal *term, Frame *root_frame);
void set_style(Terminal *term, const StyleMap *styles, const TermStyle *style);
void set_builtin_style(Terminal *term, const StyleMap *styles, BuiltinStyleEnum s);
void mask_style(TermStyle *style, const TermStyle *over);
void start_update(Terminal *term);
void end_update(EditorState *e);
void normal_update(EditorState *e);
void restore_cursor(EditorState *e);

// ui-cmdline.c
void update_command_line(EditorState *e);
void show_message(Terminal *term, const StyleMap *styles, const char *msg, bool is_error);

// ui-tabbar.c
void print_tabbar(Terminal *term, const StyleMap *styles, Window *window);

// ui-status.c
void update_status_line(const Window *window);

// ui-view.c
void update_range(EditorState *e, const View *view, long y1, long y2);

// ui-window.c
typedef enum {
    WINSEP_BLANK, // ASCII space; ' '
    WINSEP_BAR,   // ASCII vertical bar; '|'
} WindowSeparatorType;

void update_all_windows(EditorState *e);
void update_buffer_windows(EditorState *e, const Buffer *buffer);
void update_window_separators(EditorState *e);

// ui-prompt.c
char status_prompt(EditorState *e, const char *question, const char *choices) NONNULL_ARGS;
char dialog_prompt(EditorState *e, const char *question, const char *choices) NONNULL_ARGS;

#endif
