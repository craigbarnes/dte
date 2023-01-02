#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include "editor.h"
#include "terminal/key.h"
#include "util/macros.h"

bool handle_input(EditorState *e, KeyCode key) NONNULL_ARGS;

#endif
