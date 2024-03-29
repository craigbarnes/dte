#ifndef UTIL_EXITCODE_H
#define UTIL_EXITCODE_H

// Semantic exit codes, as defined by BSD sysexits(3)
enum {
    EX_OK = 0, // Exited normally
    EX_USAGE = 64, // Command line usage error
    EX_DATAERR = 65, // Input data error
    EX_OSERR = 71, // Operating system error
    EX_IOERR = 74, // Input/output error
    EX_CONFIG = 78, // Configuration error
};

// This exists purely for "self-documentation" purposes. Functions
// returning this type should only return one of the above values.
typedef int ExitCode;

#endif
