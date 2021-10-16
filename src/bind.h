#ifndef BIND_H
#define BIND_H

#include <stdbool.h>
#include "command/cache.h"
#include "command/run.h"
#include "mode.h"
#include "terminal/key.h"
#include "util/ptr-array.h"
#include "util/string.h"

void bindings_init(void);
void add_binding(InputMode mode, KeyCode key, const char *command);
void remove_binding(InputMode mode, KeyCode key);
const CachedCommand *lookup_binding(InputMode mode, KeyCode key);
bool handle_binding(InputMode mode, KeyCode key);
void collect_bound_keys(PointerArray *a, const char *keystr_prefix);
String dump_bindings(void);

#endif
