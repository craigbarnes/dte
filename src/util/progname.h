#ifndef UTIL_PROGNAME_H
#define UTIL_PROGNAME_H

#include "macros.h"

// Get the program name from argv[0] (as passed to main()), or fall back to a
// pre-defined string if it's missing
static inline const char *progname(int argc, char *argv[], const char *fallback)
{
    // Setting argv[0] properly is under the control of the parent process,
    // so it's safest to make as few assumptions about it as possible
    if (likely(argc >= 1 && argv && argv[0] && argv[0][0])) {
        return argv[0];
    }
    return fallback ? fallback : "_PROG_";
}

#endif
