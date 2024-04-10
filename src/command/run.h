#ifndef COMMAND_RUN_H
#define COMMAND_RUN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "util/macros.h"
#include "util/string-view.h"

typedef struct {
    char **args; // Positional args, with flag args moved to the front
    size_t nr_args; // Number of args (not including flag args)
    uint_least64_t flag_set; // Bitset of used flags
    char flags[10]; // Flags in parsed order
    uint8_t nr_flags; // Number of parsed flags
    uint8_t nr_flag_args; // Number of flag args
} CommandArgs;

// A set of flags associated with a Command that define how it may be
// used in certain contexts (completely unrelated to CommandArgs::flags
// or Command::flags, despite the terminology being somewhat ambiguous)
typedef enum {
    CMDOPT_ALLOW_IN_RC = 1 << 0,
} CommandOptions;

typedef bool (*CommandFunc)(void *userdata, const CommandArgs *args);

typedef struct {
    const char name[15];
    const char flags[14];
    uint8_t cmdopts; // CommandOptions
    uint8_t min_args;
    uint8_t max_args; // 0xFF here means "no limit" (effectively SIZE_MAX)
    CommandFunc cmd;
} Command;

#define CMD(cname, flagstr, opts, min, max, func) { \
    .name = cname, \
    .flags = flagstr, \
    .cmdopts = opts, \
    .min_args = min, \
    .max_args = max, \
    .cmd = (CommandFunc)func, \
}

typedef struct {
    const Command* (*lookup)(const char *name);
    void (*macro_record)(const Command *cmd, char **args, void *userdata);
} CommandSet;

typedef struct {
    const CommandSet *cmds;
    const char* (*lookup_alias)(const char *name, void *userdata);
    char* (*expand_variable)(const char *name, const void *userdata);
    const StringView *home_dir;
    void *userdata;
    unsigned int recursion_count;
    bool allow_recording;
} CommandRunner;

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
extern const Command *current_command;

static inline int command_cmp(const void *key, const void *elem)
{
    const char *name = key;
    const Command *cmd = elem;
    return strcmp(name, cmd->name);
}

bool handle_command(CommandRunner *runner, const char *cmd) NONNULL_ARGS;

#endif
