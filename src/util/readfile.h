#ifndef UTIL_READFILE_H
#define UTIL_READFILE_H

#include <sys/types.h>
#include "macros.h"

ssize_t read_file(const char *filename, char **bufp) NONNULL_ARGS WARN_UNUSED_RESULT;
ssize_t read_file_with_limit(const char *filename, char **bufp, size_t size_limit) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
