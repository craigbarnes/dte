#ifndef TERMINAL_INPUT_H
#define TERMINAL_INPUT_H

#include "key.h"
#include "terminal.h"
#include "util/macros.h"

KeyCode term_read_input(Terminal *term, unsigned int esc_timeout_ms) NONNULL_ARGS;

#endif
