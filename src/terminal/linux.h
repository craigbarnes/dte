#ifndef TERMINAL_LINUX_H
#define TERMINAL_LINUX_H

#include <stddef.h>
#include "key.h"
#include "util/macros.h"

size_t linux_parse_key(const char *buf, size_t length, KeyCode *k) WARN_UNUSED_RESULT WRITEONLY(3);

#endif
