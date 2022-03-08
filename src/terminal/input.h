#ifndef TERMINAL_INPUT_H
#define TERMINAL_INPUT_H

#include <stddef.h>
#include "key.h"
#include "terminal.h"

KeyCode term_read_key(Terminal *term, unsigned int esc_timeout);
char *term_read_paste(TermInputBuffer *ibuf, size_t *size);
void term_discard_paste(TermInputBuffer *ibuf);

#endif
