#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "debug.h"
#include "macros.h"

typedef enum {
    LOG_LEVEL_INVALID = -1,
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,
} LogLevel;

#define LOG(level, ...) log_msg(level, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...) LOG(LOG_LEVEL_CRITICAL, __VA_ARGS__)
#define LOG_ERROR(...) LOG(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_ERRNO(prefix) LOG_ERROR("%s: %s", prefix, strerror(errno))
#define LOG_WARNING(...) LOG(LOG_LEVEL_WARNING, __VA_ARGS__)
#define LOG_NOTICE(...) LOG(LOG_LEVEL_NOTICE, __VA_ARGS__)
#define LOG_INFO(...) LOG(LOG_LEVEL_INFO, __VA_ARGS__)

#if DEBUG >= 2
    #define LOG_DEBUG(...) LOG(LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
    static inline PRINTF(1) void LOG_DEBUG(const char* UNUSED_ARG(fmt), ...) {}
#endif

#if DEBUG >= 3
    #define LOG_TRACE(...) LOG(LOG_LEVEL_TRACE, __VA_ARGS__)
#else
    static inline PRINTF(1) void LOG_TRACE(const char* UNUSED_ARG(fmt), ...) {}
#endif

LogLevel log_open(const char *filename, LogLevel level, bool use_color);
bool log_close(void);
void log_msg(LogLevel level, const char *file, int line, const char *fmt, ...) PRINTF(4);
void log_msgv(LogLevel level, const char *file, int line, const char *fmt, va_list ap) VPRINTF(4);
LogLevel log_level_default(void);
LogLevel log_level_from_str(const char *str);
const char *log_level_to_str(LogLevel level);
bool log_level_enabled(LogLevel level);

#endif
