#include <string.h>
#include "trace.h"
#include "util/array.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/str-util.h"
#include "util/xstring.h"

static const char trace_names[][8] = {
    "command",
    "input",
    "status",
};

UNITTEST {
    CHECK_STRING_ARRAY(trace_names);
}

// Example valid input: "command"
static TraceLoggingFlags trace_flags_from_single_str(const char *name)
{
    for (size_t i = 0; i < ARRAYLEN(trace_names); i++) {
        if (streq(name, trace_names[i])) {
            return 1u << i;
        }
    }

    LOG_WARNING("unrecognized trace flag: '%s'", name);
    return 0;
}

// Example valid input: "command,status,input"
TraceLoggingFlags trace_flags_from_str(const char *flag_str)
{
    if (!flag_str || flag_str[0] == '\0') {
        return 0;
    }

    if (streq(flag_str, "all")) {
        return TRACE_ALL;
    }

    // Copy flag_str into a mutable buffer, so that get_delim_str() can be
    // used to split (and null-terminate) the comma-delimited substrings
    char buf[128];
    const char *end = memccpy(buf, flag_str, '\0', sizeof(buf));
    if (!end) {
        LOG_ERROR("trace flags string too long");
        return 0;
    }

    BUG_ON(end <= buf);
    TraceLoggingFlags flags = 0;

    for (size_t pos = 0, len = end - buf - 1; pos < len; ) {
        const char *piece = get_delim_str(buf, &pos, len, ',');
        flags |= trace_flags_from_single_str(piece);
    }

    return flags;
}
