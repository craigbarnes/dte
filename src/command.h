#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

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
    CMDERR_UNCLOSED_SQUOTE,
    CMDERR_UNCLOSED_DQUOTE,
    CMDERR_UNEXPECTED_EOF,
} CommandParseError;

static inline int command_cmp(const void *key, const void *elem)
{
    const char *name = key;
    const Command *cmd = elem;
    return strcmp(name, cmd->name);
}

// command-parse.c
char *parse_command_arg(const char *cmd, size_t len, bool tilde);
size_t find_end(const char *cmd, size_t pos, CommandParseError *err);
CommandParseError parse_commands(PointerArray *array, const char *cmd);
const char *command_parse_error_to_string(CommandParseError err);
void string_append_escaped_arg(String *s, const char *arg, bool escape_tilde);
char *escape_command_arg(const char *arg, bool escape_tilde);

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
