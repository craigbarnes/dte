#include <string.h>
#include "trace.h"
#include "util/array.h"
#include "util/debug.h"
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

// Example valid input: "command,status,input"
static TraceLoggingFlags trace_flags_from_str(const char *str)
{
    if (!str || str[0] == '\0') {
        return 0;
    }
    bool all = streq(str, "all");
    return all ? TRACEFLAGS_ALL : STR_TO_BITFLAGS(str, trace_names, true);
}

UNITTEST {
    CHECK_STRING_ARRAY(trace_names);
    // NOLINTBEGIN(bugprone-assert-side-effect)
    BUG_ON(trace_flags_from_str("output") != TRACEFLAG_OUTPUT);
    BUG_ON(trace_flags_from_str(",x,, ,output,,") != TRACEFLAG_OUTPUT);
    BUG_ON(trace_flags_from_str("command,input") != (TRACEFLAG_COMMAND | TRACEFLAG_INPUT));
    BUG_ON(trace_flags_from_str("command,inpu") != TRACEFLAG_COMMAND);
    BUG_ON(trace_flags_from_str("") != 0);
    BUG_ON(trace_flags_from_str(",") != 0);
    BUG_ON(trace_flags_from_str("a") != 0);
    // NOLINTEND(bugprone-assert-side-effect)
}

bool set_trace_logging_flags(const char *flag_str)
{
    if (!log_level_enabled(LOG_LEVEL_TRACE)) {
        return false;
    }

    BUG_ON(trace_flags); // Should only be called once
    trace_flags = trace_flags_from_str(flag_str);
    return (trace_flags != 0);
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
