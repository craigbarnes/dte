#ifndef BIND_H
#define BIND_H

#include "command/run.h"
#include "terminal/key.h"
#include "util/string.h"

typedef struct {
    // The cached command and parsed arguments (NULL if not cached)
    const Command *cmd;
    CommandArgs a;
    // The original command string
    char cmd_str[];
} KeyBinding;

void add_binding(const char *keystr, const char *command);
void remove_binding(const char *keystr);
const KeyBinding *lookup_binding(KeyCode key);
void handle_binding(KeyCode key);
void collect_bound_keys(const char *keystr_prefix);
String dump_bindings(void);

#endif
