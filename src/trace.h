#ifndef TRACE_H
#define TRACE_H

#include "util/log.h"

enum {
    TRACEFLAG_COMMAND = 1u << 0,
    TRACEFLAG_INPUT = 1u << 1,
    TRACEFLAG_OUTPUT = 1u << 2,
    TRACEFLAGS_ALL = 0xFFFF,
};

// Concise wrappers around log_trace() for the above flags
#define TRACE_CMD(...) LOG_TRACE(TRACEFLAG_COMMAND, __VA_ARGS__)
#define TRACE_INPUT(...) LOG_TRACE(TRACEFLAG_INPUT, __VA_ARGS__)
#define TRACE_OUTPUT(...) LOG_TRACE(TRACEFLAG_OUTPUT, __VA_ARGS__)

TraceLoggingFlags trace_flags_from_str(const char *str);

#endif
