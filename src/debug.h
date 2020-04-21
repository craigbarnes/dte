#ifndef DEBUG_H
#define DEBUG_H

#include "util/macros.h"

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

#ifdef DEBUG_PRINT
    #define DEBUG_LOG(...) debug_log(__func__, __VA_ARGS__)
    void debug_log(const char *function, const char *fmt, ...) PRINTF(2);
#else
    static inline PRINTF(1) void DEBUG_LOG(const char* UNUSED_ARG(fmt), ...) {}
#endif

noreturn void fatal_error(const char *msg, int err) COLD NONNULL_ARGS;
void term_cleanup(void);

#endif
