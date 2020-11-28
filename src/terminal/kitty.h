#ifndef TERMINAL_KITTY_H
#define TERMINAL_KITTY_H

#include <sys/types.h>
#include "key.h"

ssize_t kitty_parse_full_mode_key(const char *buf, size_t length, size_t i, KeyCode *k);

#endif
