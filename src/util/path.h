#ifndef UTIL_PATH_H
#define UTIL_PATH_H

#include "macros.h"

char *path_absolute(const char *filename) MALLOC NONNULL_ARGS;
char *relative_filename(const char *f, const char *cwd) XMALLOC NONNULL_ARGS;
char *path_dirname(const char *filename) XMALLOC NONNULL_ARGS;
const char *path_basename(const char *filename) NONNULL_ARGS RETURNS_NONNULL;

#endif
