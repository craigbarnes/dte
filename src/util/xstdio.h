#ifndef UTIL_XSTDIO_H
#define UTIL_XSTDIO_H

#include <stdbool.h>
#include <stdio.h>
#include "macros.h"
#include "xreadwrite.h"

static inline FILE *xfopen(const char *path, const char *mode, int flags, mode_t mask)
{
    BUG_ON(mode[0] == '\0');
    bool plus = (mode[1] == '+');
    BUG_ON(mode[plus ? 2 : 1] != '\0');

    switch (mode[0]) {
    case 'a':
        flags |= (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
        break;
    case 'r':
        flags |= (plus ? O_RDWR : O_RDONLY);
        break;
    case 'w':
        flags |= (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
        break;
    default:
        BUG("Invalid fopen() mode string: '%s'", mode);
    }

    int fd = xopen(path, flags, mask);
    if (fd < 0) {
        return NULL;
    }

    FILE *file = fdopen(fd, mode);
    if (!file) {
        xclose(fd);
    }
    return file;
}

#endif
