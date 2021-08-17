#ifndef TERMINAL_LINUX_H
#define TERMINAL_LINUX_H

#include <sys/types.h>
#include "key.h"

ssize_t linux_parse_key(const char *buf, size_t length, KeyCode *k);

#endif
