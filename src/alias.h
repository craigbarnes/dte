#ifndef ALIAS_H
#define ALIAS_H

#include "util/hashmap.h"
#include "util/macros.h"
#include "util/string.h"

void add_alias(HashMap *aliases, const char *name, const char *value) NONNULL_ARGS;
void remove_alias(HashMap *aliases, const char *name) NONNULL_ARGS;
const char *find_alias(const HashMap *aliases, const char *name) NONNULL_ARGS;
void collect_aliases(const char *prefix);
String dump_aliases(void);

#endif
