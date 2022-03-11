#ifndef SPAWN_H
#define SPAWN_H

#include <stdbool.h>
#include "compiler.h"
#include "msg.h"
#include "util/string.h"
#include "util/string-view.h"

typedef enum {
    SPAWN_QUIET = 1 << 0, // Interpret SPAWN_TTY as SPAWN_NULL and don't yield terminal to child
    SPAWN_PROMPT = 1 << 1, // Show "press any key to continue" prompt
    SPAWN_READ_STDOUT = 1 << 2, // Read errors from stdout instead of stderr
} SpawnFlags;

typedef enum {
    SPAWN_NULL,
    SPAWN_TTY,
    SPAWN_PIPE,
} SpawnAction;

typedef struct {
    char **argv;
    const char **env;
    StringView input;
    String output;
    SpawnFlags flags;
} SpawnContext;

bool spawn(SpawnContext *ctx, SpawnAction actions[3]);
void spawn_compiler(char **args, SpawnFlags flags, const Compiler *c, MessageArray *msgs);

#endif
