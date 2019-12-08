#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include <stdbool.h>
#include "command.h"
#include "util/macros.h"

typedef enum {
    ARGERR_INVALID_OPTION,
    ARGERR_TOO_MANY_OPTIONS,
    ARGERR_OPTION_ARGUMENT_NOT_SEPARATE,
    ARGERR_OPTION_ARGUMENT_MISSING,
    ARGERR_TOO_FEW_ARGUMENTS,
    ARGERR_TOO_MANY_ARGUMENTS,
} ArgParseErrorType;

typedef struct {
    ArgParseErrorType type;
    char flag;
} ArgParseError;

bool parse_args(const Command *cmd, CommandArgs *a) NONNULL_ARGS;
bool do_parse_args(const Command *cmd, CommandArgs *a, ArgParseError *err) NONNULL_ARG(1, 2);

#endif
