#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "debug.h"
#include "errorcode.h"
#include "macros.h"

// NOLINTNEXTLINE(readability-enum-initial-value,cert-int09-c)
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
#define LOG_ERRNO(prefix) log_errno(__FILE__, __LINE__, prefix)
#define LOG_WARNING(...) LOG(LOG_LEVEL_WARNING, __VA_ARGS__)
#define LOG_NOTICE(...) LOG(LOG_LEVEL_NOTICE, __VA_ARGS__)
#define LOG_INFO(...) LOG(LOG_LEVEL_INFO, __VA_ARGS__)
#define WARN_ON(cond) if (unlikely(cond)) {LOG_WARNING("%s", #cond);}
#define LOG_ERRNO_ON(cond, prefix) if (unlikely(cond)) {LOG_ERRNO(prefix);}

bool log_level_enabled(LogLevel level);

#if DEBUG >= 2
    #define DEBUG_LOGGING_ENABLED 1
    #define LOG_DEBUG(...) LOG(LOG_LEVEL_DEBUG, __VA_ARGS__)
    static inline bool log_level_debug_enabled(void) {return log_level_enabled(LOG_LEVEL_DEBUG);}
#else
    #define DEBUG_LOGGING_ENABLED 0
    static inline PRINTF(1) void LOG_DEBUG(const char* UNUSED_ARG(fmt), ...) {}
    static inline bool log_level_debug_enabled(void) {return false;}
#endif

#if DEBUG >= 3
    // See also: src/trace.h
    #define TRACE_LOGGING_ENABLED 1
#else
    #define TRACE_LOGGING_ENABLED 0
#endif

LogLevel log_open(const char *filename, LogLevel level, bool use_color);
bool log_close(void);
void log_msg(LogLevel level, const char *file, int line, const char *fmt, ...) PRINTF(4);
void log_msgv(LogLevel level, const char *file, int line, const char *fmt, va_list ap) VPRINTF(4);
void log_write(LogLevel level, const char *str, size_t len);
SystemErrno log_errno(const char *file, int line, const char *prefix) COLD NONNULL_ARGS;
LogLevel log_level_default(void);
LogLevel log_level_from_str(const char *str);
const char *log_level_to_str(LogLevel level);

#endif
