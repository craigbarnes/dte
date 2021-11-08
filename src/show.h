#ifndef SHOW_H
#define SHOW_H

#include <stdbool.h>
#include "editor.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

void show(EditorState *e, const char *type, const char *key, bool cflag) NONNULL_ARG(1, 2);
void collect_show_subcommands(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_show_subcommand_args(PointerArray *a, const char *name, const char *arg_prefix) NONNULL_ARGS;
void collect_env(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_normal_aliases(PointerArray *a, const char *prefix) NONNULL_ARGS;
String dump_normal_aliases(void);

#endif
