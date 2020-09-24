#ifndef COMMAND_RUN_H
#define COMMAND_RUN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint64_t flag_set; // Bitset of used flags
    char flags[8]; // Flags in parsed order
    char **args; // Positional args, with flag args moved to the front
    size_t nr_flags;
    size_t nr_args;
} CommandArgs;

typedef struct {
    const char name[16];
    const char flags[12];
    bool allow_in_rc;
    uint8_t min_args;
    uint8_t max_args; // 0xFF here means "no limit" (effectively SIZE_MAX)
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
