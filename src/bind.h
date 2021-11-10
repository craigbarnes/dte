#ifndef BIND_H
#define BIND_H

#include <stdbool.h>
#include "command/cache.h"
#include "command/run.h"
#include "terminal/key.h"
#include "util/intmap.h"
#include "util/ptr-array.h"
#include "util/string.h"

typedef struct {
    const CommandSet *cmds;
    IntMap map;
} KeyBindingGroup;

void bindings_init(void);
void add_binding(KeyBindingGroup *kbg, KeyCode key, const char *command);
void remove_binding(KeyBindingGroup *kbg, KeyCode key);
const CachedCommand *lookup_binding(KeyBindingGroup *kbg, KeyCode key);
bool handle_binding(KeyBindingGroup *kbg, KeyCode key);
void collect_bound_keys(PointerArray *a, const char *keystr_prefix);
String dump_bindings(void);

#endif
