#ifndef COMMAND_RUN_H
#define COMMAND_RUN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "util/hashmap.h"

typedef struct {
    char **args; // Positional args, with flag args moved to the front
    size_t nr_args; // Number of args (not including flag args)
    uint_least64_t flag_set; // Bitset of used flags
    char flags[10]; // Flags in parsed order
    uint8_t nr_flags; // Number of parsed flags
    uint8_t nr_flag_args; // Number of flag args
} CommandArgs;

typedef void (*CommandFunc)(void *userdata, const CommandArgs *args);

typedef struct {
    const char name[15];
    const char flags[14];
    bool allow_in_rc;
    uint8_t min_args;
    uint8_t max_args; // 0xFF here means "no limit" (effectively SIZE_MAX)
    CommandFunc cmd;
} Command;

typedef struct {
    const Command* (*lookup)(const char *name);
    void (*macro_record)(const Command *cmd, char **args, void *userdata);
    bool (*expand_variable)(const char *name, char **value, void *userdata);
    HashMap aliases;
    void *userdata;
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
