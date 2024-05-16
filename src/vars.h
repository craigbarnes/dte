#ifndef VARS_H
#define VARS_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/ptr-array.h"

struct EditorState;

char *expand_normal_var(const struct EditorState *e, const char *name) NONNULL_ARGS;
void collect_normal_vars(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
