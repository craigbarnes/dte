#ifndef TERM_H
#define TERM_H

#include <stdbool.h>
#include <stddef.h>
#include "key.h"

void term_raw(void);
void term_cooked(void);

bool term_read_key(Key *key);
char *term_read_paste(size_t *size);
void term_discard_paste(void);

bool term_get_size(int *w, int *h);

#endif
