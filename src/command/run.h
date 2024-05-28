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

struct EditorState;

typedef bool (*CommandFunc)(struct EditorState *e, const CommandArgs *args);

typedef struct {
    const char name[15];
    const char flags[14];
    uint8_t cmdopts; // CommandOptions
    uint8_t min_args;
    uint8_t max_args; // 0xFF here means "no limit" (effectively SIZE_MAX)
    CommandFunc cmd;
} Command;

typedef struct {
    const Command* (*lookup)(const char *name);
    void (*macro_record)(struct EditorState *e, const Command *cmd, char **args);
} CommandSet;

typedef struct {
    const CommandSet *cmds;
    const char* (*lookup_alias)(const struct EditorState *e, const char *name);
    char* (*expand_variable)(const struct EditorState *e, const char *name);
    const StringView *home_dir;
    struct EditorState *e;
    unsigned int recursion_count;
    bool allow_recording;
    bool expand_tilde_slash;
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
