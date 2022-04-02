#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include "macros.h"

typedef enum {
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
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

void log_init(const char *varname, LogLevel level);
void log_msg(LogLevel level, const char *file, int line, const char *fmt, ...) PRINTF(4);
LogLevel log_level_from_str(const char *str);

#endif
