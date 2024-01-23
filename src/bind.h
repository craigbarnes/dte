#ifndef BIND_H
#define BIND_H

#include <stdbool.h>
#include "command/cache.h"
#include "editor.h"
#include "mode.h"
#include "terminal/key.h"
#include "util/intmap.h"
#include "util/macros.h"
#include "util/string.h"

void add_binding(IntMap *bindings, KeyCode key, CachedCommand *cc) NONNULL_ARGS;
void remove_binding(IntMap *bindings, KeyCode key) NONNULL_ARGS;
const CachedCommand *lookup_binding(const IntMap *bindings, KeyCode key) NONNULL_ARGS;
bool handle_binding(EditorState *e, const ModeHandler *handler, KeyCode key) NONNULL_ARGS WARN_UNUSED_RESULT;

void free_bindings(IntMap *bindings) NONNULL_ARGS;
bool dump_bindings(const IntMap *bindings, const char *flag, String *buf) NONNULL_ARGS;

#endif
