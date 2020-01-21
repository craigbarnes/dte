#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>
#include "cmdline.h"
#include "mode.h"
#include "options.h"
#include "encoding/encoding.h"
#include "util/macros.h"
#include "util/ptr-array.h"

typedef enum {
    EDITOR_INITIALIZING,
    EDITOR_RUNNING,
    EDITOR_EXITING,
} EditorStatus;

typedef enum {
    INPUT_NORMAL,
    INPUT_COMMAND,
    INPUT_SEARCH,
} InputMode;

typedef struct {
    EditorStatus status;
    const EditorModeOps *mode_ops[4];
    InputMode input_mode;
    CommandLine cmdline;
    GlobalOptions options;
    const char *home_dir;
    const char *user_config_dir;
    Encoding charset;
    const char *pager;
    bool child_controls_terminal;
    bool everything_changed;
    bool term_utf8;
    size_t cmdline_x;
    PointerArray buffers;
    PointerArray search_history;
    PointerArray command_history;
    const char *const version;
} EditorState;

extern EditorState editor;

static inline void mark_everything_changed(void)
{
    editor.everything_changed = true;
}

static inline void set_input_mode(InputMode mode)
{
    editor.input_mode = mode;
}

void init_editor_state(void);
char *editor_file(const char *name) XSTRDUP;
char status_prompt(const char *question, const char *choices) NONNULL_ARGS;
char dialog_prompt(const char *question, const char *choices) NONNULL_ARGS;
void any_key(void);
void normal_update(void);
void handle_sigwinch(int signum);
void suspend(void);
void main_loop(void);
void ui_resize(void);
void ui_start(void);
void ui_end(void);

#endif
