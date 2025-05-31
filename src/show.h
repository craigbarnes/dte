#ifndef SHOW_H
#define SHOW_H

#include <stdbool.h>
#include "editor.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

typedef String (*DumpFunc)(EditorState *e);

bool show(EditorState *e, const char *type, const char *key, bool cflag) NONNULL_ARG(1) WARN_UNUSED_RESULT;
void collect_show_subcommands(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_show_subcommand_args(EditorState *e, PointerArray *a, const char *name, const char *arg_prefix) NONNULL_ARGS;
DumpFunc get_dump_function(const char *name) NONNULL_ARGS;

#endif
