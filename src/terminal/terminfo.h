#ifndef TERMINAL_TERMINFO_H
#define TERMINAL_TERMINFO_H

#include <stdbool.h>
#include "../util/macros.h"

bool term_init_terminfo(const char *term) NONNULL_ARGS;

#endif
