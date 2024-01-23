#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include "command/run.h"
#include "terminal/key.h"
#include "util/hashmap.h"
#include "util/intmap.h"
#include "util/macros.h"
#include "util/ptr-array.h"

typedef struct {
    const char *name;
    const CommandSet *cmds;
    IntMap key_bindings;
    bool insert_text_for_unicode_range;
    PointerArray fallthrough_modes;
} ModeHandler;

static inline ModeHandler *get_mode_handler(HashMap *modes, const char *name)
{
    return hashmap_get(modes, name);
}

struct EditorState;
bool handle_input(struct EditorState *e, KeyCode key) NONNULL_ARGS;
ModeHandler *new_mode(HashMap *modes, char *name, const CommandSet *cmds) NONNULL_ARGS_AND_RETURN;

#endif
