#ifndef INPUT_SPECIAL_H
#define INPUT_SPECIAL_H

#include <stdbool.h>
#include <stddef.h>
#include "term.h"

void special_input_activate(void);
bool special_input_keypress(Key key, char *buf, int *count);
bool special_input_misc_status(char *status, size_t status_maxlen);

#endif
