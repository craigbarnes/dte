#ifndef EXEC_H
#define EXEC_H

#include <stdbool.h>
#include <sys/types.h>
#include "editor.h"
#include "spawn.h"
#include "util/macros.h"

// Note: these need to be kept sorted
typedef enum {
    EXEC_BUFFER,
    EXEC_ERRMSG,
    EXEC_EVAL,
    EXEC_LINE,
    EXEC_MSG,
    EXEC_NULL,
    EXEC_OPEN,
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

#endif
