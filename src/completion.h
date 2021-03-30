#ifndef COMPLETION_H
#define COMPLETION_H

#include "util/hashmap.h"
#include "util/macros.h"

void complete_command_next(void);
void complete_command_prev(void);
void reset_completion(void);
void add_completion(char *str);
void collect_hashmap_keys(const HashMap *map, const char *prefix) NONNULL_ARGS;

#endif
