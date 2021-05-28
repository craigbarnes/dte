#ifndef MODE_H
#define MODE_H

#include "terminal/key.h"

typedef enum {
    INPUT_NORMAL,
    INPUT_COMMAND,
    INPUT_SEARCH,
} InputMode;

void handle_input(KeyCode key);

#endif
