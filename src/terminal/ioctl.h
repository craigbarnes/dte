#ifndef TERMINAL_IOCTL_H
#define TERMINAL_IOCTL_H

#include <stdbool.h>
#include "util/macros.h"

bool term_get_size(unsigned int *w, unsigned int *h) NONNULL_ARGS WARN_UNUSED_RESULT;
bool term_drop_controlling_tty(int fd);

#endif
