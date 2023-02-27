#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"
#include "cmdline.h"
#include "command/macro.h"
#include "command/run.h"
#include "commands.h"
#include "copy.h"
#include "file-history.h"
#include "frame.h"
#include "history.h"
#include "msg.h"
#include "options.h"
#include "regexp.h"
#include "search.h"
#include "syntax/color.h"
#include "tag.h"
#include "terminal/cursor.h"
#include "terminal/terminal.h"
#include "util/debug.h"
#include "util/hashmap.h"
#include "util/intmap.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "view.h"

typedef enum {
    EDITOR_INITIALIZING = -2,
    EDITOR_RUNNING = -1,
    // Values 0-125 are exit codes
    EDITOR_EXIT_OK = 0,
    EDITOR_EXIT_MAX = 125,
} EditorStatus;

typedef enum {
    INPUT_NORMAL,
    INPUT_COMMAND,
    INPUT_SEARCH,
} InputMode;

typedef struct {
    const CommandSet *cmds;
    IntMap key_bindings;
} ModeHandler;

typedef struct EditorState {
    EditorStatus status;
    InputMode input_mode;
    CommandLine cmdline;
    SearchState search;
    GlobalOptions options;
    Terminal terminal;
    StringView home_dir;
    const char *user_config_dir;
    bool child_controls_terminal;
    bool everything_changed;
    bool cursor_style_changed;
    bool session_leader;
    size_t cmdline_x;
    ModeHandler modes[3];
    Clipboard clipboard;
    TagFile tagfile;
    HashMap aliases;
    HashMap compilers;
    HashMap syntaxes;
    ColorScheme colors;
    CommandMacroState macro;
    TermCursorStyle cursor_styles[NR_CURSOR_MODES];
    Frame *root_frame;
    struct Window *window;
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
    RegexpWordBoundaryTokens regexp_word_tokens;
    const char *version;
} EditorState;

static inline void mark_everything_changed(EditorState *e)
{
    e->everything_changed = true;
}

static inline void set_input_mode(EditorState *e, InputMode mode)
{
    e->cursor_style_changed = true;
    e->input_mode = mode;
}

static inline CommandRunner cmdrunner_for_mode(EditorState *e, InputMode mode, bool allow_recording)
{
    BUG_ON(mode >= ARRAYLEN(e->modes));
    CommandRunner runner = {
        .cmds = e->modes[mode].cmds,
        .lookup_alias = (mode == INPUT_NORMAL) ? find_normal_alias : NULL,
        .home_dir = &e->home_dir,
        .allow_recording = allow_recording,
        .userdata = e,
    };
    return runner;
}

EditorState *init_editor_state(void) RETURNS_NONNULL;
void free_editor_state(EditorState *e) NONNULL_ARGS;
void any_key(Terminal *term, unsigned int esc_timeout) NONNULL_ARGS;
int main_loop(EditorState *e) NONNULL_ARGS WARN_UNUSED_RESULT;
void ui_start(EditorState *e) NONNULL_ARGS;
void ui_end(EditorState *e) NONNULL_ARGS;
void ui_resize(EditorState *e) NONNULL_ARGS;

#endif
