#ifndef UTIL_EXITCODE_H
#define UTIL_EXITCODE_H

#include <stddef.h>
#include "macros.h"

// Semantic exit codes, as defined by BSD sysexits(3)
enum {
    EC_OK = 0, // Exited normally (EX_OK)
    EC_USAGE_ERROR = 64, // Command line usage error (EX_USAGE)
    EC_DATA_ERROR = 65, // Input data error (EX_DATAERR)
    EC_OS_ERROR = 71, // Operating system error (EX_OSERR)
    EC_IO_ERROR = 74, // Input/output error (EX_IOERR)
    EC_CONFIG_ERROR = 78, // Configuration error (EX_CONFIG)
};

// This exists purely for "self-documentation" purposes. Functions
// returning this type should only return one of the above values.
typedef int ExitCode;

ExitCode ec_error(const char *prefix, ExitCode ec) COLD;
ExitCode ec_write_stdout(const char *str, size_t len);
ExitCode ec_usage_error(const char *restrict fmt, ...) PRINTF(1) NONNULL_ARGS COLD;
ExitCode ec_io_error(const char *restrict fmt, ...) PRINTF(1) NONNULL_ARGS COLD;
ExitCode ec_os_error(const char *restrict fmt, ...) PRINTF(1) NONNULL_ARGS COLD;
ExitCode ec_printf_ok(const char *restrict fmt, ...) PRINTF(1) NONNULL_ARGS;

#endif
