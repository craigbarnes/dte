#ifndef ENV_H
#define ENV_H

#include <stdbool.h>

void collect_builtin_env(const char *prefix);
bool expand_builtin_env(const char *name, char **value);

#endif
