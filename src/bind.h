#ifndef BIND_H
#define BIND_H

#include "key.h"

void add_binding(const char *keys, const char *command);
void remove_binding(const char *keys);
void handle_binding(Key key);
int nr_pressed_keys(void);

#endif
