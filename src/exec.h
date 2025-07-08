#ifndef EXEC_H
#define EXEC_H

#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include "command/error.h"
#include "editor.h"
#include "spawn.h"
#include "util/macros.h"
#include "util/ptr-array.h"

extern volatile sig_atomic_t child_controls_terminal;

// NOLINTNEXTLINE(readability-enum-initial-value,cert-int09-c)
typedef enum {
    EXEC_INVALID = -1,
    // Note: items below here need to be kept sorted (for exec_map[])
    EXEC_BUFFER = 0,
    EXEC_COMMAND,
    EXEC_ECHO,
    EXEC_ERRMSG,
    EXEC_EVAL,
    EXEC_LINE,
    EXEC_MSG,
    EXEC_MSG_A,
    EXEC_MSG_B,
    EXEC_MSG_C,
    EXEC_NULL,
    EXEC_OPEN,
    EXEC_OPEN_REL,
    EXEC_SEARCH,
    EXEC_TAG,
    EXEC_TAG_A,
    EXEC_TAG_B,
    EXEC_TAG_C,
    EXEC_TTY,
    EXEC_WORD,
} ExecAction;

typedef enum {
    EXECFLAG_QUIET = 1u << 0, // Interpret SPAWN_TTY as SPAWN_NULL and don't yield terminal to child
    EXECFLAG_PROMPT = 1u << 1, // Show "press any key to continue" prompt
    EXECFLAG_STRIP_NL = 1u << 2, // Strip newline from end of inserted text (if any)
    EXECFLAG_BUILTIN = 1u << 3, // Look up argv[0] as a built-in script, instead of in the filesystem
} ExecFlags;

ssize_t handle_exec (
    EditorState *e,
    const char **argv,
    ExecAction actions[3],
    ExecFlags flags
) NONNULL_ARGS;

ExecAction lookup_exec_action(const char *name, int fd) NONNULL_ARGS;
int open_builtin_script(ErrorBuffer *ebuf, const char *name) NONNULL_ARGS WARN_UNUSED_RESULT;
void yield_terminal(EditorState *e, bool quiet) NONNULL_ARGS;
void resume_terminal(EditorState *e, bool quiet, bool prompt) NONNULL_ARGS;
void collect_exec_actions(PointerArray *a, const char *prefix, int fd) NONNULL_ARGS;

#endif
