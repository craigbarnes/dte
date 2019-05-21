#ifndef UTIL_PATH_H
#define UTIL_PATH_H

#include "macros.h"

char *path_absolute(const char *filename) MALLOC NONNULL_ARGS;
char *relative_filename(const char *f, const char *cwd) XSTRDUP;
char *path_dirname(const char *filename) XSTRDUP;
const char *path_basename(const char *filename) NONNULL_ARGS_AND_RETURN;

#endif
