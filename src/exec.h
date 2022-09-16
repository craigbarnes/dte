#ifndef EXEC_H
#define EXEC_H

#include <stdbool.h>
#include <sys/types.h>
#include "editor.h"
#include "spawn.h"
#include "util/macros.h"

typedef enum {
    EXEC_INVALID = -1,
    // Note: items below here need to be kept sorted
    EXEC_BUFFER = 0,
    EXEC_COMMAND,
    EXEC_ERRMSG,
    EXEC_EVAL,
    EXEC_LINE,
    EXEC_MSG,
    EXEC_NULL,
    EXEC_OPEN,
    EXEC_SEARCH,
    EXEC_TAG,
    EXEC_TTY,
    EXEC_WORD,
} ExecAction;

ssize_t handle_exec (
    EditorState *e,
    const char **argv,
    ExecAction actions[3],
    SpawnFlags spawn_flags,
    bool strip_trailing_newline
) NONNULL_ARGS;

ExecAction lookup_exec_action(const char *name, int fd) NONNULL_ARGS;

#endif
