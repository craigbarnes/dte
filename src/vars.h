#ifndef VARS_H
#define VARS_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/ptr-array.h"

char *expand_normal_var(const char *name, const void *userdata) NONNULL_ARGS;
void collect_normal_vars(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
