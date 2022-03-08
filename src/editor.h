#ifndef EDITOR_H
#define EDITOR_H

#include <regex.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include "bind.h"
#include "buffer.h"
#include "cmdline.h"
#include "copy.h"
#include "encoding.h"
#include "file-history.h"
#include "frame.h"
#include "history.h"
#include "msg.h"
#include "options.h"
#include "syntax/color.h"
#include "terminal/terminal.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "view.h"

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

typedef enum {
    SEARCH_FWD,
    SEARCH_BWD,
} SearchDirection;

typedef struct {
    regex_t regex;
    char *pattern;
    SearchDirection direction;
    int re_flags; // If zero, regex hasn't been compiled
} SearchState;

typedef struct Window {
    PointerArray views;
    Frame *frame;
    View *view; // Current view
    View *prev_view; // Previous view, if set
    int x, y; // Coordinates for top left of window
    int w, h; // Width and height of window (including tabbar and status)
    int edit_x, edit_y; // Top left of editable area
    int edit_w, edit_h; // Width and height of editable area
    size_t first_tab_idx;
    bool update_tabbar;
    struct {
        int width;
        long first;
        long last;
    } line_numbers;
} Window;

typedef struct {
    EditorStatus status;
    InputMode input_mode;
    CommandLine cmdline;
    SearchState search;
    GlobalOptions options;
    Terminal terminal;
    StringView home_dir;
    const char *user_config_dir;
    const char *file_locks;
    const char *file_locks_lock;
    mode_t file_locks_mode;
    bool child_controls_terminal;
    bool everything_changed;
    bool session_leader;
    pid_t pid;
    int exit_code;
    size_t cmdline_x;
    KeyBindingGroup bindings[3];
    Clipboard clipboard;
    HashMap compilers;
    HashMap syntaxes;
    ColorScheme colors;
    Frame *root_frame;
    Window *window;
    View *view;
    Buffer *buffer;
    PointerArray buffers;
    PointerArray filetypes;
    PointerArray file_options;
    PointerArray bookmarks;
    MessageArray messages;
    FileHistory file_history;
    History search_history;
    History command_history;
    const char *const version;
    volatile sig_atomic_t resized;
} EditorState;

extern EditorState editor;

static inline void mark_everything_changed(EditorState *e)
{
    e->everything_changed = true;
}

static inline void set_input_mode(EditorState *e, InputMode mode)
{
    e->input_mode = mode;
}

void init_editor_state(void);
void free_editor_state(EditorState *e);
const char *editor_file(const char *name) NONNULL_ARGS_AND_RETURN;
char status_prompt(EditorState *e, const char *question, const char *choices) NONNULL_ARGS;
char dialog_prompt(EditorState *e, const char *question, const char *choices) NONNULL_ARGS;
void any_key(EditorState *e);
void normal_update(EditorState *e);
void main_loop(EditorState *e);
void ui_start(EditorState *e);
void ui_end(EditorState *e);

#endif
