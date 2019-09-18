#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "util/ptr-array.h"

typedef struct {
    char flags[8];
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

typedef enum {
    CMDERR_NONE,
    CMDERR_UNCLOSED_SINGLE_QUOTE,
    CMDERR_UNCLOSED_DOUBLE_QUOTE,
    CMDERR_UNEXPECTED_EOF,
} CommandParseError;

#define CMD_ARG_MAX 4095 // (1 << 12) - 1

// command-parse.c
char *parse_command_arg(const char *cmd, size_t len, bool tilde);
size_t find_end(const char *cmd, size_t pos, CommandParseError *err);
bool parse_commands(PointerArray *array, const char *cmd, CommandParseError *err);
const char *command_parse_error_to_string(CommandParseError err);

// command-run.c
extern const Command *current_command;
const Command *find_command(const Command *cmds, const char *name);
void run_commands(const Command *cmds, const PointerArray *array);
void run_command(const Command *cmds, char **argv);
void handle_command(const Command *cmds, const char *cmd);

// command.c
extern const Command commands[];

#endif
