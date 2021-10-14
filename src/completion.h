#ifndef COMPLETION_H
#define COMPLETION_H

#include "cmdline.h"
#include "util/hashmap.h"
#include "util/macros.h"
#include "util/ptr-array.h"

void complete_command_next(CommandLine *cmdline) NONNULL_ARGS;
void complete_command_prev(CommandLine *cmdline) NONNULL_ARGS;
void reset_completion(CommandLine *cmdline) NONNULL_ARGS;

void collect_hashmap_keys(const HashMap *map, PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
