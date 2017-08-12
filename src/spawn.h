#ifndef SPAWN_H
#define SPAWN_H

#include "compiler.h"

// Errors are read from stderr by default.
#define SPAWN_READ_STDOUT (1 << 0)

// Redirect to /dev/null?
#define SPAWN_QUIET (1 << 2)

// Press any key to continue
#define SPAWN_PROMPT (1 << 4)

typedef struct {
    char *in;
    char *out;
    long in_len;
    long out_len;
} FilterData;

int spawn_filter(char **argv, FilterData *data);
void spawn_compiler(char **args, unsigned int flags, Compiler *c);
void spawn(char **args, int fd[3], bool prompt);

#endif
