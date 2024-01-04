#ifndef UTIL_PROGNAME_H
#define UTIL_PROGNAME_H

#include "macros.h"

static inline const char *progname(int argc, char *argv[], const char *fallback)
{
    if (likely(argc >= 1 && argv && argv[0] && argv[0][0])) {
        return argv[0];
    }
    return fallback ? fallback : "_PROG_";
}

#endif
