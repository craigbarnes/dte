#ifndef VARS_H
#define VARS_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/ptr-array.h"

bool expand_normal_var(const char *name, char **value) NONNULL_ARGS;
bool expand_syntax_var(const char *name, char **value) NONNULL_ARGS;
void collect_normal_vars(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
