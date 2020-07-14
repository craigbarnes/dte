#ifndef SPAWN_H
#define SPAWN_H

#include <stdbool.h>
#include "compiler.h"
#include "util/string.h"
#include "util/string-view.h"

typedef enum {
    SPAWN_DEFAULT = 0,
    SPAWN_READ_STDOUT = 1 << 0, // Read errors from stdout instead of stderr
    SPAWN_QUIET = 1 << 2, // Redirect streams to /dev/null
    SPAWN_PROMPT = 1 << 4 // Show "press any key to continue" prompt
} SpawnFlags;

typedef struct {
    char **argv;
    const char **env;
    StringView input;
    String output;
    SpawnFlags flags;
} SpawnContext;

bool spawn_source(SpawnContext *ctx);
bool spawn_sink(SpawnContext *ctx);
bool spawn_filter(SpawnContext *ctx);
void spawn(SpawnContext *ctx);
void spawn_compiler(char **args, SpawnFlags flags, const Compiler *c);

#endif
