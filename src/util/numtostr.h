#ifndef UTIL_NUMTOSTR_H
#define UTIL_NUMTOSTR_H

#include <stddef.h>
#include <stdint.h>
#include "macros.h"

size_t buf_umax_to_str(uintmax_t x, char *buf) NONNULL_ARGS;
size_t buf_uint_to_str(unsigned int x, char *buf) NONNULL_ARGS;
size_t buf_ulong_to_str(unsigned long x, char *buf) NONNULL_ARGS;

const char *umax_to_str(uintmax_t x) RETURNS_NONNULL;
const char *uint_to_str(unsigned int x) RETURNS_NONNULL;
const char *ulong_to_str(unsigned long x) RETURNS_NONNULL;

#endif
