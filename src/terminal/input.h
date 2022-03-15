#ifndef TERMINAL_INPUT_H
#define TERMINAL_INPUT_H

#include <stddef.h>
#include "key.h"
#include "terminal.h"
#include "util/string.h"

void term_input_init(TermInputBuffer *ibuf);
void term_input_free(TermInputBuffer *ibuf);
KeyCode term_read_key(Terminal *term, unsigned int esc_timeout);
char *term_read_detected_paste(TermInputBuffer *input, size_t *size);
void term_discard_detected_paste(TermInputBuffer *input);
String term_read_bracketed_paste(TermInputBuffer *input);
void term_discard_bracketed_paste(TermInputBuffer *input);

#endif
