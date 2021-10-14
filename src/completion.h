#ifndef COMPLETION_H
#define COMPLETION_H

#include "cmdline.h"
#include "util/hashmap.h"
#include "util/macros.h"

void complete_command_next(CommandLine *cmdline);
void complete_command_prev(CommandLine *cmdline);
void reset_completion(CommandLine *cmdline);
void add_completion(char *str);
void collect_hashmap_keys(const HashMap *map, const char *prefix) NONNULL_ARGS;

#endif
