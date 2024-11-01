#ifndef UTIL_XMEMRCHR_H
#define UTIL_XMEMRCHR_H

#include <stddef.h>
#include "macros.h"

void *xmemrchr(const void *ptr, int c, size_t n) PURE NONNULL_ARGS;

#endif
