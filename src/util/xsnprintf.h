#ifndef UTIL_XSNPRINTF_H
#define UTIL_XSNPRINTF_H

#include <stdarg.h>
#include <stddef.h>
#include "macros.h"

size_t xsnprintf(char *restrict buf, size_t n, const char *restrict fmt, ...) PRINTF(3) NONNULL_ARGS;
size_t xvsnprintf(char *restrict buf, size_t n, const char *restrict fmt, va_list v) VPRINTF(3) NONNULL_ARGS;

#endif
