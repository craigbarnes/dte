#ifndef TERMINAL_INPUT_H
#define TERMINAL_INPUT_H

#include <stddef.h>
#include "key.h"

KeyCode term_read_key(void);
char *term_read_paste(size_t *size);
void term_discard_paste(void);

#endif
