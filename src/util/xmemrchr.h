#ifndef UTIL_XMEMRCHR_H
#define UTIL_XMEMRCHR_H

#include <stddef.h>
#include "macros.h"

void *xmemrchr(const void *ptr, int c, size_t n) PURE NONNULL_ARG_IF_NONZERO_LENGTH(1, 3);

#endif
