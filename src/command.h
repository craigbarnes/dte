#ifndef COMMAND_H
#define COMMAND_H

#include <sys/types.h> // size_t, ssize_t
#include <stdbool.h>
#include "ptr-array.h"
#include "error.h"

typedef struct {
    const char *name;
    const char *flags;
    signed char min_args;
    signed char max_args;
    void (*cmd)(const char *, char **);
} Command;

// parse-command.c
char *parse_command_arg(const char *cmd, bool tilde);
ssize_t find_end(const char *cmd, size_t pos, Error **err);
bool parse_commands(PointerArray *array, const char *cmd, Error **err);
char **copy_string_array(char **src, size_t count);

// run.c
extern const Command *current_command;

const Command *find_command(const Command *cmds, const char *name);
void run_commands(const Command *cmds, const PointerArray *array);
void handle_command(const Command *cmds, const char *cmd);

// commands.c
extern const Command commands[];

#endif
