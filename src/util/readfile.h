#ifndef UTIL_READFILE_H
#define UTIL_READFILE_H

#include <sys/types.h>
#include "macros.h"

// Returns size of file or -1 on error.
// For empty files *bufp is NULL, otherwise *bufp is NUL-terminated.
ssize_t read_file(const char *filename, char **bufp) NONNULL_ARG(1);

#endif
