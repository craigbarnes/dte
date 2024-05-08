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
#include "mode.h"
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
    EFLAG_SAVE_CMD_HIST = 1u << 0, // Update command history on quit
    EFLAG_SAVE_SEARCH_HIST = 1u << 1, // Update search history on quit
    EFLAG_SAVE_FILE_HIST = 1u << 2, // Update file history on quit
    EFLAG_SAVE_ALL_HIST = (EFLAG_SAVE_FILE_HIST << 1) - 1, // All of the above
} EditorFlags;

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
    Terminal terminal;
    StringView home_dir;
    const char *user_config_dir;
    mode_t new_file_mode;
    EditorFlags flags;
    bool child_controls_terminal;
    bool everything_changed;
    bool cursor_style_changed;
    bool session_leader;
    size_t cmdline_x;
    Clipboard clipboard;
    TagFile tagfile;
    HashMap aliases;
    HashMap compilers;
    HashMap modes;
    HashMap syntaxes;
    HashSet required_syntax_files;
    HashSet required_syntax_builtins;
    StyleMap styles;
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

static inline void set_input_mode(EditorState *e, ModeHandler *mode)
{
    if (e->mode != mode) {
        e->mode = mode;
        e->cursor_style_changed = true;
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

static inline CommandRunner cmdrunner (
    EditorState *e,
    const CommandSet *cmds,
    bool allow_recording
) {
    bool normal = (cmds == &normal_commands);
    return (CommandRunner) {
        .cmds = cmds,
        .lookup_alias = normal ? find_normal_alias : NULL,
        .expand_variable = normal ? expand_normal_var : NULL,
        .home_dir = &e->home_dir,
        .userdata = e,
        .allow_recording = allow_recording,
    };
}

static inline CommandRunner normal_mode_cmdrunner(EditorState *e, bool allow_recording)
{
    return cmdrunner(e, &normal_commands, allow_recording);
}

EditorState *init_editor_state(void) RETURNS_NONNULL;
void free_editor_state(EditorState *e) NONNULL_ARGS;
void any_key(Terminal *term, unsigned int esc_timeout) NONNULL_ARGS;
int main_loop(EditorState *e) NONNULL_ARGS WARN_UNUSED_RESULT;
void ui_start(EditorState *e) NONNULL_ARGS;
void ui_end(EditorState *e) NONNULL_ARGS;
void ui_resize(EditorState *e) NONNULL_ARGS;

#endif
