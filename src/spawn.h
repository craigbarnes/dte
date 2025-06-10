#ifndef SPAWN_H
#define SPAWN_H

#include <stdbool.h>
#include "compiler.h"
#include "msg.h"
#include "util/macros.h"
#include "util/string.h"
#include "util/string-view.h"

typedef enum {
    SPAWN_NULL,
    SPAWN_TTY,
    SPAWN_PIPE,
} SpawnAction;

typedef struct {
    const char **argv;
    StringView input;
    String outputs[2]; // For stdout/stderr
    SpawnAction actions[3];
    ErrorBuffer *ebuf;
    unsigned int lines;
    unsigned int columns;
    bool quiet;
} SpawnContext;

int spawn(SpawnContext *ctx) NONNULL_ARGS WARN_UNUSED_RESULT;
bool spawn_compiler(SpawnContext *ctx, const Compiler *c, MessageArray *msgs, bool read_stdout) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
