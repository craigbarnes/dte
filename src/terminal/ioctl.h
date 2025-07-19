#ifndef TERMINAL_IOCTL_H
#define TERMINAL_IOCTL_H

#include "util/errorcode.h"
#include "util/macros.h"

SystemErrno term_get_size(unsigned int *w, unsigned int *h) NONNULL_ARGS WARN_UNUSED_RESULT;
SystemErrno term_drop_controlling_tty(int fd);

#endif
