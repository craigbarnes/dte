#ifndef TERMINAL_INPUT_H
#define TERMINAL_INPUT_H

#include <stdbool.h>
#include "key.h"
#include "terminal.h"
#include "util/string.h"

void term_input_init(TermInputBuffer *ibuf);
void term_input_free(TermInputBuffer *ibuf);
KeyCode term_read_key(Terminal *term, unsigned int esc_timeout);
String term_read_paste(TermInputBuffer *input, bool bracketed);
void term_discard_paste(TermInputBuffer *input, bool bracketed);

#endif
