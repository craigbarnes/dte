#ifndef UTIL_STRTONUM_H
#define UTIL_STRTONUM_H

#include <stdbool.h>
#include <stddef.h>
#include "macros.h"

int number_width(long n) CONST_FN;
bool buf_parse_long(const char *str, size_t size, size_t *posp, long *valp) NONNULL_ARG(1, 3);
bool parse_long(const char **strp, long *valp) NONNULL_ARG(1);
bool str_to_long(const char *str, long *valp) NONNULL_ARG(1);
bool str_to_int(const char *str, int *valp) NONNULL_ARG(1);

#endif
