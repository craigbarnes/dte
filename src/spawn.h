#ifndef SPAWN_H
#define SPAWN_H

#include <stdbool.h>
#include <stddef.h>
#include "compiler.h"
#include "util/string.h"

typedef enum {
    SPAWN_DEFAULT = 0,
    SPAWN_READ_STDOUT = 1 << 0, // Read errors from stdout instead of stderr
    SPAWN_QUIET = 1 << 2, // Redirect streams to /dev/null
    SPAWN_PROMPT = 1 << 4 // Show "press any key to continue" prompt
} SpawnFlags;

typedef struct {
    char *in;
    char *out;
    size_t in_len;
    size_t out_len;
} FilterData;

#define FILTER_DATA_INIT { \
    .in = NULL, \
    .out = NULL, \
    .in_len = 0, \
    .out_len = 0 \
}

int spawn_source(char **argv, String *output);
int spawn_sink(char **argv, const char *text, size_t length);
int spawn_filter(char **argv, FilterData *data);
void spawn_compiler(char **args, SpawnFlags flags, const Compiler *c);
void spawn(char **args, bool quiet, bool prompt);

#endif
