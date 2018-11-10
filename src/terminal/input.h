#ifndef TERMINAL_INPUT_H
#define TERMINAL_INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include "key.h"

bool term_read_key(KeyCode *key);
char *term_read_paste(size_t *size);
void term_discard_paste(void);

bool term_get_size(int *w, int *h);

#endif
