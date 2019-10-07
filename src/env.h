#ifndef ENV_H
#define ENV_H

#include <stdbool.h>
#include "util/macros.h"

void collect_builtin_env(const char *prefix) NONNULL_ARGS;
bool expand_builtin_env(const char *name, char **value) NONNULL_ARGS;

#endif
