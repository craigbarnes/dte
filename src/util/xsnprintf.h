#ifndef UTIL_XSNPRINTF_H
#define UTIL_XSNPRINTF_H

#include <stdarg.h>
#include <stddef.h>
#include "macros.h"

PRINTF(3) NONNULL_ARGS
int xsnprintf(char *restrict buf, size_t len, const char *restrict fmt, ...);

VPRINTF(3) NONNULL_ARGS
int xvsnprintf(char *restrict buf, size_t len, const char *restrict fmt, va_list ap);

#endif
