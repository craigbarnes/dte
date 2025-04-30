#ifndef TRACE_H
#define TRACE_H

#include "util/log.h"

enum {
    TRACE_COMMAND = 1u << 0,
    TRACE_INPUT = 1u << 1,
    TRACE_STATUSLINE = 1u << 2,
    TRACE_ALL = 0xFFFF,
};

TraceLoggingFlags trace_flags_from_str(const char *str);

#endif
