#ifndef BIND_H
#define BIND_H

#include <stdbool.h>
#include "command/run.h"
#include "mode.h"
#include "terminal/key.h"
#include "util/string.h"

typedef struct {
    // The cached command and parsed arguments (NULL if not cached)
    const Command *cmd;
    CommandArgs a;
    // The original command string
    char cmd_str[];
} KeyBinding;

void bindings_init(void);
void add_binding(InputMode mode, KeyCode key, const char *command);
void remove_binding(InputMode mode, KeyCode key);
const KeyBinding *lookup_binding(InputMode mode, KeyCode key);
bool handle_binding(InputMode mode, KeyCode key);
void collect_bound_keys(const char *keystr_prefix);
String dump_bindings(void);

#endif
