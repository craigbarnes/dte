#ifndef COMMAND_ALIAS_H
#define COMMAND_ALIAS_H

#include "util/hashmap.h"
#include "util/macros.h"

static inline const char *find_alias(const HashMap *aliases, const char *name)
{
    return hashmap_get(aliases, name);
}

void add_alias(HashMap *aliases, const char *name, const char *value) NONNULL_ARGS;
void remove_alias(HashMap *aliases, const char *name) NONNULL_ARGS;

#endif
