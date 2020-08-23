#ifndef COMMAND_RUN_H
#define COMMAND_RUN_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "util/bitset.h"

typedef struct {
    BitSetWord flag_set[BITSET_NR_WORDS(128)]; // BitSet of used flags
    char flags[8]; // Flags in parsed order
    char **args; // Positional args, with flag args moved to the front
    size_t nr_flags;
    size_t nr_args;
} CommandArgs;

typedef struct {
    const char name[15];
    const char flags[8];
    bool allow_in_rc;
    unsigned int min_args;
    unsigned int max_args;
    void (*cmd)(const CommandArgs *args);
} Command;

typedef struct {
    const Command* (*lookup)(const char *name);
    bool (*allow_recording)(const Command *cmd, char **args);
} CommandSet;

extern const Command *current_command;

static inline int command_cmp(const void *key, const void *elem)
{
    const char *name = key;
    const Command *cmd = elem;
    return strcmp(name, cmd->name);
}

void handle_command(const CommandSet *cmds, const char *cmd, bool allow_recording);

#endif
