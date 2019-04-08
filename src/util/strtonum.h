#ifndef UTIL_STRTONUM_H
#define UTIL_STRTONUM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "macros.h"

int number_width(long n) CONST_FN;
size_t buf_parse_uintmax(const char *str, size_t size, uintmax_t *valp) NONNULL_ARG(1);
size_t buf_parse_long(const char *str, size_t size, long *valp) NONNULL_ARG(1);
bool parse_long(const char **strp, long *valp) NONNULL_ARG(1);
bool str_to_int(const char *str, int *valp) NONNULL_ARG(1);
bool str_to_size(const char *str, size_t *valp) NONNULL_ARG(1);

#endif
