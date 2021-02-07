#ifndef UTIL_DEBUG_H
#define UTIL_DEBUG_H

#include "macros.h"

#define BUG_ON(a) do { \
    IGNORE_WARNING("-Wtautological-compare") \
    if (unlikely(a)) { \
        BUG("%s", #a); \
    } \
    UNIGNORE_WARNINGS \
} while (0)

#if DEBUG >= 1
    #define BUG(...) bug(__FILE__, __LINE__, __func__, __VA_ARGS__)
    noreturn void bug(const char *file, int line, const char *func, const char *fmt, ...) COLD PRINTF(4);
#else
    #define BUG(...) UNREACHABLE()
#endif

#if DEBUG >= 2
    #define DEBUG_LOG(...) debug_log(__FILE__, __LINE__, __VA_ARGS__)
    void debug_log(const char *file, int line, const char *fmt, ...) PRINTF(3);
    void log_init(const char *varname);
#else
    static inline PRINTF(1) void DEBUG_LOG(const char* UNUSED_ARG(fmt), ...) {}
    static inline void log_init(const char* UNUSED_ARG(varname)) {}
#endif

noreturn void fatal_error(const char *msg, int err) COLD NONNULL_ARGS;
noreturn void init_error(const char *fmt, ...) COLD NONNULL_ARGS PRINTF(1);
void set_fatal_error_cleanup_handler(void (*handler)(void));

#endif
