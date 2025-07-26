#include "trace.h"
#include "util/array.h"
#include "util/str-util.h"
#include "util/xstring.h"

#if TRACE_LOGGING_ENABLED

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
static TraceLoggingFlags trace_flags = 0;

static const char trace_names[][8] = {
    "command",
    "input",
    "output",
};

UNITTEST {
    CHECK_STRING_ARRAY(trace_names);
}

// Example valid input: "command,status,input"
TraceLoggingFlags trace_flags_from_str(const char *str)
{
    if (!str || str[0] == '\0') {
        return 0;
    }

    bool all = streq(str, "all");
    return all ? TRACEFLAGS_ALL : STR_TO_BITFLAGS(str, trace_names, true);
}

void set_trace_logging_flags(TraceLoggingFlags flags)
{
    BUG_ON(trace_flags); // Should be called at most once
    BUG_ON(!flags); // ... with non-zero flags
    BUG_ON(!log_level_enabled(LOG_LEVEL_TRACE)); // ... and only when enabled
    trace_flags = flags;
}

// Return true if *any* 1 bits in `flags` are enabled in `trace_flags`
bool log_trace_enabled(TraceLoggingFlags flags)
{
    return !!(trace_flags & flags);
}

void log_trace(TraceLoggingFlags flags, const char *file, int line, const char *fmt, ...)
{
    if (!log_trace_enabled(flags)) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    log_msgv(LOG_LEVEL_TRACE, file, line, fmt, ap);
    va_end(ap);
}

#endif // TRACE_LOGGING_ENABLED
