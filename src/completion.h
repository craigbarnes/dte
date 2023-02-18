#ifndef COMPLETION_H
#define COMPLETION_H

#include "cmdline.h"
#include "editor.h"
#include "util/hashmap.h"
#include "util/macros.h"
#include "util/ptr-array.h"

void complete_command_next(EditorState *e) NONNULL_ARGS;
void complete_command_prev(EditorState *e) NONNULL_ARGS;
void reset_completion(CommandLine *cmdline) NONNULL_ARGS;

void collect_env(EditorState *e, PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_normal_aliases(EditorState *e, PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_bound_normal_keys(EditorState *e, PointerArray *a, const char *keystr_prefix) NONNULL_ARGS;
void collect_hl_colors(EditorState *e, PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_compilers(EditorState *e, PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_hashmap_keys(const HashMap *map, PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
