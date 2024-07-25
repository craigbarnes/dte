#ifndef TERMINAL_INPUT_H
#define TERMINAL_INPUT_H

#include "key.h"
#include "terminal.h"
#include "util/macros.h"

void term_input_init(TermInputBuffer *ibuf) NONNULL_ARGS;
void term_input_free(TermInputBuffer *ibuf) NONNULL_ARGS;
KeyCode term_read_input(Terminal *term, unsigned int esc_timeout_ms) NONNULL_ARGS;

#endif
