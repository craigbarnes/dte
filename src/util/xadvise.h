#ifndef UTIL_XADVISE_H
#define UTIL_XADVISE_H

#include <stddef.h>
#include "macros.h"

int advise_sequential(void *addr, size_t len) NONNULL_ARGS;

#endif
