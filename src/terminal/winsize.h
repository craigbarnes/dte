#ifndef TERMINAL_WINSIZE_H
#define TERMINAL_WINSIZE_H

#include <stdbool.h>
#include "util/macros.h"

bool term_get_size(unsigned int *w, unsigned int *h) NONNULL_ARGS;

#endif
