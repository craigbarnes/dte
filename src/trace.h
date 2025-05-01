#ifndef TRACE_H
#define TRACE_H

#include "util/log.h"

enum {
    TRACE_FLAG_COMMAND = 1u << 0,
    TRACE_FLAG_INPUT = 1u << 1,
    TRACE_FLAG_OUTPUT = 1u << 2,
    TRACE_FLAGS_ALL = 0xFFFF,
};

// Concise wrappers around log_trace() for the above flags
#define TRACE_CMD(...) LOG_TRACE(TRACE_FLAG_COMMAND, __VA_ARGS__)
#define TRACE_INPUT(...) LOG_TRACE(TRACE_FLAG_INPUT, __VA_ARGS__)
#define TRACE_OUTPUT(...) LOG_TRACE(TRACE_FLAG_OUTPUT, __VA_ARGS__)

TraceLoggingFlags trace_flags_from_str(const char *str);

#endif
