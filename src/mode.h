#ifndef MODE_H
#define MODE_H

#include "terminal/key.h"

void normal_mode_keypress(KeyCode key);
void command_mode_keypress(KeyCode key);
void search_mode_keypress(KeyCode key);

#endif
