#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "buffer.h"
#include "cmdline.h"
#include "command/error.h"
#include "command/macro.h"
#include "command/run.h"
#include "commands.h"
#include "copy.h"
#include "file-history.h"
#include "frame.h"
#include "history.h"
#include "lock.h"
#include "mode.h"
#include "msg.h"
#include "options.h"
#include "regexp.h"
#include "search.h"
#include "syntax/color.h"
#include "syntax/state.h"
#include "tag.h"
#include "terminal/cursor.h"
#include "terminal/terminal.h"
#include "util/debug.h"
#include "util/hashmap.h"
#include "util/hashset.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "vars.h"
#include "view.h"

typedef enum {
    EDITOR_INITIALIZING = -2,
    EDITOR_RUNNING = -1,
    // Values 0-125 are exit codes
    EDITOR_EXIT_OK = 0,
    EDITOR_EXIT_MAX = 125,
} EditorStatus;

typedef enum {
    EFLAG_CMD_HIST = 1u << 0, // Save command history on quit
    EFLAG_SEARCH_HIST = 1u << 1, // Save search history on quit
    EFLAG_FILE_HIST = 1u << 2, // Save file history on quit
    EFLAG_ALL_HIST = EFLAG_CMD_HIST | EFLAG_SEARCH_HIST | EFLAG_FILE_HIST,
    EFLAG_HEADLESS = 1u << 3, // Running in "headless" mode (no tty interaction)
} EditorFlags;

// Used to track which parts of the screen have changed since the last
// call to update_screen()
typedef enum {
    UPDATE_TERM_TITLE = 1u << 0, // update_term_title()
    UPDATE_CURSOR_STYLE = 1u << 1, // update_cursor_style()
    UPDATE_WINDOW_SEPARATORS = 1u << 2, // update_window_separators()

    // TODO: Set this when the buffer contents changes or when the cursor
    // position changes in a way that requires a redraw (e.g. a change of line
    // affected by `hi currentline`, `set ws-error trailing`, scrolling, etc.),
    // then handle it accordingly in update_screen()
    UPDATE_CURRENT_BUFFER = 1u << 3,

    UPDATE_ALL_WINDOWS = 1u << 4, // update_all_windows()
    UPDATE_ALL = (UPDATE_ALL_WINDOWS << 1) - 1, // All of the above

    UPDATE_DIALOG = 1u << 24, // show_dialog(); modal dialog for e.g. `quit -p`

    UPDATE_SYNTAX_STYLES = 1u << 25, // update_all_syntax_styles(); triggered by cmd_hi()
} ScreenUpdateFlags;

typedef struct EditorState {
    EditorStatus status;
    ModeHandler *mode; // Current mode
    ModeHandler *prev_mode; // Mode to use after leaving command/search mode
    ModeHandler *normal_mode;
    ModeHandler *command_mode;
    ModeHandler *search_mode;
    CommandLine cmdline;
    SearchState search;
    GlobalOptions options;
    StringView home_dir; // $HOME (interned)
    const char *user_config_dir; // $DTE_HOME or equivalent (interned)
    mode_t new_file_mode;
    EditorFlags flags;
    ScreenUpdateFlags screen_update;
    uint8_t include_recursion_count;
    bool session_leader;
    Clipboard clipboard;
    TagFile tagfile;
    HashMap aliases;
    HashMap compilers;
    HashMap modes;
    HashMap syntaxes;
    HashSet required_syntax_files;
    HashSet required_syntax_builtins;
    StyleMap styles;
    SyntaxLoader syn;
    MacroRecorder macro;
    TermCursorStyle cursor_styles[NR_CURSOR_MODES];
    FileLocksContext locks_ctx;
    Frame *root_frame;
    struct Window *window;
    View *view;
    Buffer *buffer;
    PointerArray buffers;
    PointerArray filetypes;
    PointerArray file_options;
    PointerArray bookmarks;
    MessageList messages[3];
    FileHistory file_history;
    History search_history;
    History command_history;
    RegexpWordBoundaryTokens regexp_word_tokens;
    ErrorBuffer err;
    Terminal terminal;
} EditorState;

static inline void set_input_mode(EditorState *e, ModeHandler *mode)
{
    if (e->mode != mode) {
        e->mode = mode;
        e->screen_update |= UPDATE_CURSOR_STYLE;
    }
}

static inline void push_input_mode(EditorState *e, ModeHandler *mode)
{
    if (e->mode->cmds == &normal_commands) {
        // Save the previous mode only when entering command/search mode, so
        // that the previously active mode can be returned to on accept/cancel
        // (regardless of edge cases like e.g. running `command; command`)
        e->prev_mode = e->mode;
    }
    set_input_mode(e, mode);
}

static inline void pop_input_mode(EditorState *e)
{
    BUG_ON(!e->prev_mode);
    set_input_mode(e, e->prev_mode);
}

static inline CommandRunner cmdrunner(EditorState *e, const CommandSet *cmds)
{
    bool normal = (cmds == &normal_commands);
    return (CommandRunner) {
        .cmds = cmds,
        .lookup_alias = normal ? find_normal_alias : NULL,
        .expand_variable = normal ? expand_normal_var : NULL,
        .home_dir = &e->home_dir,
        .e = e,
        .ebuf = &e->err,
        .flags = CMDRUNNER_EXPAND_TILDE_SLASH,
    };
}

static inline CommandRunner normal_mode_cmdrunner(EditorState *e)
{
    return cmdrunner(e, &normal_commands);
}

EditorState *init_editor_state(const char *home, const char *dte_home) RETURNS_NONNULL;
void free_editor_state(EditorState *e) NONNULL_ARGS;
void clear_all_messages(EditorState *e) NONNULL_ARGS;
void any_key(Terminal *term, unsigned int esc_timeout) NONNULL_ARGS;
void main_loop(EditorState *e, bool timing) NONNULL_ARGS;
void ui_first_start(EditorState *e, unsigned int terminal_query_level) NONNULL_ARGS;
void ui_start(EditorState *e) NONNULL_ARGS;
void ui_end(Terminal *term, bool final) NONNULL_ARGS;
void ui_resize(EditorState *e) NONNULL_ARGS;

#endif
