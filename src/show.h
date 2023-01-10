#ifndef SHOW_H
#define SHOW_H

#include <stdbool.h>
#include "editor.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

bool show(EditorState *e, const char *type, const char *key, bool cflag) NONNULL_ARG(1, 2) WARN_UNUSED_RESULT;
void collect_show_subcommands(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_show_subcommand_args(EditorState *e, PointerArray *a, const char *name, const char *arg_prefix) NONNULL_ARGS;

String dump_all_bindings(EditorState *e);
String dump_command_history(EditorState *e);
String dump_compilers(EditorState *e);
String dump_cursors(EditorState *e);
String dump_frames(EditorState *e);
String dump_normal_aliases(EditorState *e);
String dump_options_and_fileopts(EditorState *e);
String dump_search_history(EditorState *e);
String do_dump_builtin_configs(EditorState *e);
String do_dump_filetypes(EditorState *e);
String do_dump_hl_colors(EditorState *e);
String do_dump_options(EditorState *e);

#endif
