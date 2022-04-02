#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include "macros.h"

#if DEBUG >= 2
    #define DEBUG_LOG(...) debug_log(__FILE__, __LINE__, __VA_ARGS__)
    void debug_log(const char *file, int line, const char *fmt, ...) PRINTF(3);
    void log_init(const char *varname);
#else
    static inline PRINTF(1) void DEBUG_LOG(const char* UNUSED_ARG(fmt), ...) {}
    static inline void log_init(const char* UNUSED_ARG(varname)) {}
#endif

#endif
