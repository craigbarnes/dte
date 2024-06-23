#ifndef TERMINAL_QUERY_H
#define TERMINAL_QUERY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "key.h"
#include "parse.h"
#include "util/macros.h"

KeyCode parse_csi_query_reply(const TermControlParams *csi, uint8_t prefix) NONNULL_ARGS WARN_UNUSED_RESULT;
KeyCode parse_dcs_query_reply(const char *data, size_t len, bool truncated) NONNULL_ARGS WARN_UNUSED_RESULT;
KeyCode parse_osc_query_reply(const char *data, size_t len, bool truncated) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
