#ifndef UTIL_XSTDIO_H
#define UTIL_XSTDIO_H

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include "debug.h"
#include "macros.h"
#include "xreadwrite.h"

// Convert fopen(3) mode string to equivalent open(3) flags
static inline int xfopen_mode_to_flags(const char *mode)
{
    BUG_ON(mode[0] == '\0');
    bool plus = (mode[1] == '+');
    BUG_ON(mode[plus ? 2 : 1] != '\0');

    switch (mode[0]) {
    case 'a':
        return (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
    case 'r':
        return (plus ? O_RDWR : O_RDONLY);
    case 'w':
        return (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
    }

    BUG("Unknown fopen() mode string: '%s'", mode);
    return 0;
}

static inline FILE *xfopen(const char *path, const char *mode, int flags, mode_t mask)
{
    BUG_ON(flags != (flags & (O_CLOEXEC | O_NOCTTY | O_NOFOLLOW)));
    flags |= xfopen_mode_to_flags(mode);

    int fd = xopen(path, flags, mask);
    if (fd < 0) {
        return NULL;
    }

    FILE *file = fdopen(fd, mode);
    if (unlikely(!file)) {
        xclose(fd);
    }

    return file;
}

char *xfgets(char *restrict buf, int bufsize, FILE *restrict stream);
int xfputs(const char *restrict str, FILE *restrict stream);
int xfputc(int c, FILE *stream);
int xvfprintf(FILE *restrict stream, const char *restrict fmt, va_list ap) VPRINTF(2);
int xfprintf(FILE *restrict stream, const char *restrict fmt, ...) PRINTF(2);
int xfflush(FILE *stream);

#endif
