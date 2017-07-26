#ifndef BIND_H
#define BIND_H

void add_binding(const char *keys, const char *command);
void remove_binding(const char *keys);
void handle_binding(int key);
int nr_pressed_keys(void);

#endif
