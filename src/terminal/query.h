#ifndef TERMINAL_QUERY_H
#define TERMINAL_QUERY_H

#include <stdbool.h>
#include <stddef.h>
#include "key.h"
#include "util/macros.h"

KeyCode parse_dcs_query_reply(const char *data, size_t len, bool truncated) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
