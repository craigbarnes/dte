#ifndef UTIL_XMEMMEM_H
#define UTIL_XMEMMEM_H

#include <stddef.h>
#include "macros.h"

void *xmemmem(const void *haystack, size_t hlen, const void *needle, size_t nlen) PURE NONNULL_ARGS;

#endif
