#ifndef TERMINAL_PARSE_H
#define TERMINAL_PARSE_H

#include <sys/types.h>
#include "key.h"
#include "util/macros.h"

ssize_t term_parse_sequence(const char *buf, size_t length, KeyCode *k) WARN_UNUSED_RESULT;

#endif
