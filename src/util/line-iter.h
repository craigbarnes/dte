#ifndef UTIL_LINE_ITER_H
#define UTIL_LINE_ITER_H

#include <stddef.h>
#include "macros.h"
#include "string-view.h"

char *buf_next_line(char *buf, size_t *posp, size_t size) NONNULL_ARGS_AND_RETURN;
StringView buf_slice_next_line(const char *buf, size_t *posp, size_t size) NONNULL_ARGS;

#endif
