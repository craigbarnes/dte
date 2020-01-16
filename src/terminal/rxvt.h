#ifndef TERMINAL_RXVT_H
#define TERMINAL_RXVT_H

#include <sys/types.h>
#include "key.h"

ssize_t rxvt_parse_key(const char *buf, size_t length, KeyCode *k);

#endif
