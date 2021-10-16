#ifndef COMMAND_CACHE_H
#define COMMAND_CACHE_H

#include "command/run.h"
#include "util/macros.h"

typedef struct {
    const Command *cmd; // Cached command (or NULL if not cacheable)
    CommandArgs a; // Pre-parsed arguments (uninitialized if cmd is NULL)
    char cmd_str[]; // Original command string
} CachedCommand;

CachedCommand *cached_command_new(const CommandSet *cmds, const char *cmd_str) NONNULL_ARGS_AND_RETURN;
void cached_command_free(CachedCommand *cc);

#endif
