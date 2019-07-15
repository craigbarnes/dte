#ifndef UTIL_READFILE_H
#define UTIL_READFILE_H

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "macros.h"
#include "string-view.h"

ssize_t stat_read_file(const char *filename, char **bufp, struct stat *st);

// Returns size of file or -1 on error.
// For empty files *bufp is NULL, otherwise *bufp is NUL-terminated.
static inline ssize_t read_file(const char *filename, char **bufp)
{
    struct stat st;
    return stat_read_file(filename, bufp, &st);
}

char *buf_next_line(char *buf, size_t *posp, size_t size);
StringView buf_slice_next_line(const char *buf, size_t *posp, size_t size);

#endif
