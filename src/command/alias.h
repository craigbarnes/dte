#ifndef ALIAS_H
#define ALIAS_H

#include "util/hashmap.h"
#include "util/macros.h"

void add_alias(HashMap *aliases, const char *name, const char *value) NONNULL_ARGS;
void remove_alias(HashMap *aliases, const char *name) NONNULL_ARGS;
const char *find_alias(const HashMap *aliases, const char *name) NONNULL_ARGS;

#endif
