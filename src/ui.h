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

// A snapshot of various editor state values, as recorded by main_loop()
// and later checked by update_screen(), to determine which areas of the
// screen need to be redrawn
typedef struct {
    bool is_modified;
    unsigned long id;
    long cy;
    long vx;
    long vy;
} ScreenState;

static inline ScreenState get_screen_state(const View *view)
{
    return (ScreenState) {
        .is_modified = buffer_modified(view->buffer),
        .id = view->buffer->id,
        .cy = view->cy,
        .vx = view->vx,
        .vy = view->vy
    };
}

// ui.c
void update_screen(EditorState *e, const ScreenState *s);
void update_term_title(Terminal *term, const Buffer *buffer, bool set_window_title);
void update_window_sizes(Terminal *term, Frame *frame);
void update_screen_size(Terminal *term, Frame *root_frame);
void set_style(Terminal *term, const StyleMap *styles, const TermStyle *style);
void set_builtin_style(Terminal *term, const StyleMap *styles, BuiltinStyleEnum s);

// ui-cmdline.c
size_t update_command_line (
    Terminal *term,
    const StyleMap *styles,
    const CommandLine *cmdline,
    const SearchState *search,
    const ModeHandler *mode
);

// ui-tabbar.c
void print_tabbar(Terminal *term, const StyleMap *styles, Window *window);

// ui-status.c
void update_status_line(const Window *window);

// ui-view.c
void update_range (
    Terminal *term,
    const View *view,
    const StyleMap *styles,
    long y1,
    long y2,
    bool display_special
);

// ui-window.c
typedef enum {
    WINSEP_BLANK, // ASCII space; ' '
    WINSEP_BAR,   // ASCII vertical bar; '|'
} WindowSeparatorType;

void update_all_windows(Terminal *term, Frame *root_frame, const StyleMap *styles);
void update_buffer_windows(Terminal *term, const View *view, const StyleMap *styles, const GlobalOptions *options);
void update_window_separators(Terminal *term, Frame *root_frame, const StyleMap *styles);

// ui-prompt.c
char status_prompt(EditorState *e, const char *question, const char *choices) NONNULL_ARGS;
char dialog_prompt(EditorState *e, const char *question, const char *choices) NONNULL_ARGS;
void show_dialog(Terminal *term, const StyleMap *styles, const char *question) NONNULL_ARGS;

#endif
