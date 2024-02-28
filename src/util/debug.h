#ifndef UTIL_DEBUG_H
#define UTIL_DEBUG_H

#include <stddef.h> // unreachable()
#include "macros.h"

#ifndef DEBUG
    #define DEBUG 0
#endif

#define BUG_ON(a) do { \
    IGNORE_WARNING("-Wtautological-compare") \
    if (unlikely(a)) { \
        BUG("%s", #a); \
    } \
    UNIGNORE_WARNINGS \
} while (0)

#if GNUC_AT_LEAST(4, 5) || HAS_BUILTIN(__builtin_unreachable)
    #define UNREACHABLE() __builtin_unreachable()
#elif __STDC_VERSION__ >= 202311L
    #define UNREACHABLE() unreachable()
#else
    #define UNREACHABLE()
#endif

#if DEBUG >= 1
    #define UNITTEST CONSTRUCTOR static void XPASTE(unittest_, COUNTER)(void)
    #define BUG(...) bug(__FILE__, __LINE__, __func__, __VA_ARGS__)
    noreturn void bug(const char *file, int line, const char *func, const char *fmt, ...) COLD PRINTF(4);
#else
    #define UNITTEST UNUSED static void XPASTE(unittest_, COUNTER)(void)
    #define BUG(...) UNREACHABLE()
#endif

typedef void (*CleanupHandler)(void *userdata);

noreturn void fatal_error(const char *msg, int err) COLD NONNULL_ARGS;
void set_fatal_error_cleanup_handler(CleanupHandler handler, void *userdata);
void fatal_error_cleanup(void);

#endif
