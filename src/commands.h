#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include "command/run.h"
#include "config.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"

extern const CommandSet normal_commands;

struct EditorState;

const Command *find_normal_command(const char *name) NONNULL_ARGS;
const char *find_normal_alias(const char *name, void *userdata) NONNULL_ARGS;
bool handle_normal_command(struct EditorState *e, const char *cmd, bool allow_recording) NONNULL_ARGS;
void exec_normal_config(struct EditorState *e, StringView config) NONNULL_ARGS;
int read_normal_config(struct EditorState *e, const char *filename, ConfigFlags flags) NONNULL_ARGS;
void collect_normal_commands(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
