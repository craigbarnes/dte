#ifndef COMMAND_ARGS_H
#define COMMAND_ARGS_H

#include <stdbool.h>
#include "run.h"
#include "util/macros.h"

typedef enum {
    ARGERR_INVALID_OPTION = 1,
    ARGERR_TOO_MANY_OPTIONS,
    ARGERR_OPTION_ARGUMENT_NOT_SEPARATE,
    ARGERR_OPTION_ARGUMENT_MISSING,
    ARGERR_TOO_FEW_ARGUMENTS,
    ARGERR_TOO_MANY_ARGUMENTS,
} ArgParseErrorType;

bool parse_args(const Command *cmd, CommandArgs *a) NONNULL_ARGS;
unsigned int do_parse_args(const Command *cmd, CommandArgs *a) NONNULL_ARGS;

#endif
