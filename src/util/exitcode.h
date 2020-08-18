#ifndef UTIL_EXITCODE_H
#define UTIL_EXITCODE_H

// Semantic exit codes, as defined by BSD sysexits(3)
enum {
    EX_OK = 0, // Exited normally
    EX_USAGE = 64, // Command line usage error
    EX_DATAERR = 65, // Input data error
    EX_OSERR = 71, // Operating system error
    EX_IOERR = 74, // Input/output error
};

typedef int ExitCode;

#endif
