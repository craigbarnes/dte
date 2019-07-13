#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "error.h"
#include "util/ptr-array.h"

typedef struct {
    const char *flags;
    char **args;
    size_t nr_flags;
    size_t nr_args;
} CommandArgs;

typedef struct {
    const char name[15];
    const char flags[7];
    uint16_t min_args : 4;
    uint16_t max_args : 12;
    void (*cmd)(const CommandArgs *args);
} Command;

#define CMD_ARG_MAX 4095 // (1 << 12) - 1

// command-parse.c
char *parse_command_arg(const char *cmd, size_t len, bool tilde);
size_t find_end(const char *cmd, size_t pos, Error **err);
bool parse_commands(PointerArray *array, const char *cmd, Error **err);
char **copy_string_array(char **src, size_t count);

// command-run.c
extern const Command *current_command;
const Command *find_command(const Command *cmds, const char *name);
void run_commands(const Command *cmds, const PointerArray *array);
void handle_command(const Command *cmds, const char *cmd);

// command.c
extern const Command commands[];

#endif
