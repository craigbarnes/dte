#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "cmdline.h"
#include "command/error.h"
#include "command/run.h"
#include "frame.h"
#include "options.h"
#include "search.h"
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
    bool set_window_title;
    unsigned long id;
    long cy;
    long vx;
    long vy;
} ScreenState;

struct EditorState;

// ui.c
void update_screen(struct EditorState *e, const ScreenState *s);
void update_term_title(TermOutputBuffer *obuf, const char *filename, bool is_modified);
void update_window_sizes(Terminal *term, Frame *frame);
void update_screen_size(Terminal *term, Frame *root_frame);
void set_style(Terminal *term, const StyleMap *styles, const TermStyle *style);
void set_builtin_style(Terminal *term, const StyleMap *styles, BuiltinStyleEnum s);

// ui-cmdline.c
size_t update_command_line (
    Terminal *term,
    const ErrorBuffer *err,
    const StyleMap *styles,
    const CommandLine *cmdline,
    const SearchState *search,
    const CommandSet *cmds
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
char status_prompt(struct EditorState *e, const char *question, const char *choices) NONNULL_ARGS;
char dialog_prompt(struct EditorState *e, const char *question, const char *choices) NONNULL_ARGS;
void show_dialog(Terminal *term, const StyleMap *styles, const char *question) NONNULL_ARGS;

#endif
