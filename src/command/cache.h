#ifndef COMMAND_CACHE_H
#define COMMAND_CACHE_H

#include "command/run.h"
#include "util/macros.h"

typedef struct {
    // The cached command and parsed arguments (NULL if not cached)
    const Command *cmd;
    CommandArgs a;
    // The original command string
    char cmd_str[];
} CachedCommand;

CachedCommand *cached_command_new(const CommandSet *cmds, const char *cmd_str) NONNULL_ARGS_AND_RETURN;
void cached_command_free(CachedCommand *cc);

#endif
