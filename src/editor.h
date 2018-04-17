#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>
#include "macros.h"
#include "cmdline.h"
#include "options.h"
#include "color.h"
#include "term.h"

typedef enum {
    EDITOR_INITIALIZING,
    EDITOR_RUNNING,
    EDITOR_EXITING,
} EditorStatus;

typedef enum {
    INPUT_NORMAL,
    INPUT_COMMAND,
    INPUT_SEARCH,
    INPUT_GIT_OPEN,
} InputMode;

typedef struct {
    void (*keypress)(Key key);
    void (*update)(void);
} EditorModeOps;

typedef struct {
    EditorStatus status;
    const EditorModeOps *mode_ops[4];
    TermColor *builtin_colors[NR_BC];
    InputMode input_mode;
    CommandLine cmdline;
    GlobalOptions options;
    const char *home_dir;
    const char *user_config_dir;
    const char *charset;
    bool child_controls_terminal;
    bool everything_changed;
    bool resized;
    bool term_utf8;
    int cmdline_x;
    PointerArray search_history;
    PointerArray command_history;
    const char *version;
} EditorState;

extern EditorState editor;

void init_editor_state(void);
char *editor_file(const char *name);
char get_confirmation(const char *choices, const char *format, ...) PRINTF(2);
void set_input_mode(InputMode mode);
void any_key(void);
void normal_update(void);
void resize(void);
void ui_end(void);
void suspend(void);
void set_signal_handler(int signum, void (*handler)(int signum));
void main_loop(void);

#endif
