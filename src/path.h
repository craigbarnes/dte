#ifndef PATH_H
#define PATH_H

#include "util/macros.h"

char *path_absolute(const char *filename) MALLOC NONNULL_ARGS;
char *relative_filename(const char *f, const char *cwd) XMALLOC NONNULL_ARGS;
char *short_filename_cwd(const char *absolute, const char *cwd) XMALLOC NONNULL_ARGS;
char *short_filename(const char *absolute) XMALLOC NONNULL_ARGS;
char *path_dirname(const char *filename) XMALLOC NONNULL_ARGS;
const char *path_basename(const char *filename) NONNULL_ARGS RETURNS_NONNULL;

#endif
