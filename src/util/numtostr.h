#ifndef UTIL_NUMTOSTR_H
#define UTIL_NUMTOSTR_H

#include <stddef.h>
#include "macros.h"

const char *uint_to_str(unsigned int x) RETURNS_NONNULL;
size_t buf_uint_to_str(unsigned int x, char *buf) NONNULL_ARGS;

#endif
