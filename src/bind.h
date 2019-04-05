#ifndef BIND_H
#define BIND_H

#include <stddef.h>
#include "terminal/key.h"
#include "util/macros.h"
#include "util/string.h"

void add_binding(const char *keystr, const char *command);
void remove_binding(const char *keystr);
void handle_binding(KeyCode key);
String dump_bindings(void);

#endif
