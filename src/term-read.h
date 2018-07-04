#ifndef TERM_H
#define TERM_H

#include <stdbool.h>
#include <stddef.h>
#include "key.h"

bool term_read_key(Key *key);
char *term_read_paste(size_t *size);
void term_discard_paste(void);

bool term_get_size(int *w, int *h);

const char *term_get_last_key_escape_sequence(void);

#endif
