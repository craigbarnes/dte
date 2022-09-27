#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include "editor.h"
#include "terminal/key.h"

bool handle_input(EditorState *e, KeyCode key);

#endif
