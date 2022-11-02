#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include "macros.h"

typedef enum {
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,
} LogLevel;

#define LOG(level, ...) log_msg(level, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) LOG(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_WARNING(...) LOG(LOG_LEVEL_WARNING, __VA_ARGS__)
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

LogLevel log_init(const char *filename, LogLevel level);
void log_msg(LogLevel level, const char *file, int line, const char *fmt, ...) PRINTF(4);
void log_msgv(LogLevel level, const char *file, int line, const char *fmt, va_list ap) VPRINTF(4);
LogLevel log_level_default(void);
LogLevel log_level_from_str(const char *str);
const char *log_level_to_str(LogLevel level);

#endif
