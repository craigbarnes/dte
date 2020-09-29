#ifndef EDITOR_H
#define EDITOR_H

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include "cmdline.h"
#include "encoding.h"
#include "history.h"
#include "options.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"

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
    InputMode input_mode;
    CommandLine cmdline;
    GlobalOptions options;
    StringView home_dir;
    const char *user_config_dir;
    const char *xdg_runtime_dir;
    Encoding charset;
    bool child_controls_terminal;
    bool everything_changed;
    bool term_utf8;
    int exit_code;
    size_t cmdline_x;
    PointerArray buffers;
    History search_history;
    History command_history;
    const char *const version;
    volatile sig_atomic_t resized;
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
const char *editor_file(const char *name) NONNULL_ARGS_AND_RETURN;
char status_prompt(const char *question, const char *choices) NONNULL_ARGS;
char dialog_prompt(const char *question, const char *choices) NONNULL_ARGS;
void any_key(void);
void normal_update(void);
void suspend(void);
void main_loop(void);
void ui_start(void);
void ui_end(void);

#endif
