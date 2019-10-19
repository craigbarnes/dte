#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "util/macros.h"
#include "util/ptr-array.h"

typedef struct {
    char flags[8];
    char **args;
    size_t nr_flags;
    size_t nr_args;
} CommandArgs;

typedef struct {
    const char name[16];
    const char flags[8];
    unsigned int min_args;
    unsigned int max_args;
    void (*cmd)(const CommandArgs *args);
} Command;

typedef struct {
    const Command* (*lookup)(const char *name);
} CommandSet;

typedef enum {
    CMDERR_NONE,
    CMDERR_UNCLOSED_SINGLE_QUOTE,
    CMDERR_UNCLOSED_DOUBLE_QUOTE,
    CMDERR_UNEXPECTED_EOF,
} CommandParseError;

// command-parse.c
char *parse_command_arg(const char *cmd, size_t len, bool tilde);
size_t find_end(const char *cmd, size_t pos, CommandParseError *err);
bool parse_commands(PointerArray *array, const char *cmd, CommandParseError *err);
const char *command_parse_error_to_string(CommandParseError err);

// command-run.c
extern const Command *current_command;
void run_commands(const CommandSet *cmds, const PointerArray *array);
void run_command(const CommandSet *cmds, char **argv);
void handle_command(const CommandSet *cmds, const char *cmd);

// command.c
extern const CommandSet commands;
const Command *find_normal_command(const char *name) NONNULL_ARGS;
void collect_normal_commands(const char *prefix) NONNULL_ARGS;

#endif
