#ifndef TERMINAL_XTERM_H
#define TERMINAL_XTERM_H

#include <sys/types.h>
#include "key.h"

ssize_t xterm_parse_key(const char *buf, size_t length, KeyCode *k);

#endif
