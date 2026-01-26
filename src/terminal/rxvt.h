#ifndef TERMINAL_RXVT_H
#define TERMINAL_RXVT_H

#include <sys/types.h>
#include "key.h"
#include "util/macros.h"

ssize_t rxvt_parse_key(const char *buf, size_t length, KeyCode *k) WARN_UNUSED_RESULT WRITEONLY(3);

#endif
