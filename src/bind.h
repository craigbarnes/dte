#ifndef BIND_H
#define BIND_H

#include <stddef.h>
#include "terminal/key.h"

void add_binding(const char *keys, const char *command);
void remove_binding(const char *keys);
void handle_binding(KeyCode key);
size_t nr_pressed_keys(void);

#endif
