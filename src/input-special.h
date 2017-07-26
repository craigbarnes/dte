#ifndef INPUT_SPECIAL_H
#define INPUT_SPECIAL_H

#include "term.h"
#include "libc.h"

void special_input_activate(void);
bool special_input_keypress(int key, char *buf, int *count);
bool special_input_misc_status(char *status);

#endif
