#ifndef TRACE_H
#define TRACE_H

#include <stdbool.h>
#include "util/log.h"
#include "util/macros.h"

typedef enum {
    TRACEFLAG_COMMAND = 1u << 0,
    TRACEFLAG_INPUT = 1u << 1,
    TRACEFLAG_OUTPUT = 1u << 2,
    TRACEFLAGS_ALL = 0xFFFF,
} TraceLoggingFlags;

// Concise wrappers around log_trace(), for the above flags
#define TRACE_CMD(...) LOG_TRACE(TRACEFLAG_COMMAND, __VA_ARGS__)
#define TRACE_INPUT(...) LOG_TRACE(TRACEFLAG_INPUT, __VA_ARGS__)
#define TRACE_OUTPUT(...) LOG_TRACE(TRACEFLAG_OUTPUT, __VA_ARGS__)

#if TRACE_LOGGING_ENABLED
    #define LOG_TRACE(flags, ...) log_trace(flags, __FILE__, __LINE__, __VA_ARGS__)
    void log_trace(TraceLoggingFlags flags, const char *file, int line, const char *fmt, ...) PRINTF(4);
    bool log_trace_enabled(TraceLoggingFlags flags);
    void set_trace_logging_flags(TraceLoggingFlags flags);
#else
    static inline PRINTF(2) void LOG_TRACE(TraceLoggingFlags UNUSED_ARG(flags), const char* UNUSED_ARG(fmt), ...) {}
    static inline bool log_trace_enabled(TraceLoggingFlags UNUSED_ARG(flags)) {return false;}
    static inline void set_trace_logging_flags(TraceLoggingFlags UNUSED_ARG(flags)) {}
#endif

TraceLoggingFlags trace_flags_from_str(const char *str);

#endif
