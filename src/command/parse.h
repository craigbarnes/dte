#ifndef COMMAND_PARSE_H
#define COMMAND_PARSE_H

#include <stdbool.h>
#include <stddef.h>
#include "command/run.h"
#include "util/macros.h"
#include "util/ptr-array.h"

typedef enum {
    CMDERR_NONE,
    CMDERR_UNCLOSED_SQUOTE,
    CMDERR_UNCLOSED_DQUOTE,
    CMDERR_UNEXPECTED_EOF,
} CommandParseError;

char *parse_command_arg(const CommandRunner *runner, const char *cmd, size_t len, bool tilde) NONNULL_ARG(1) RETURNS_NONNULL;
size_t find_end(const char *cmd, size_t pos, CommandParseError *err) NONNULL_ARGS;
CommandParseError parse_commands(const CommandRunner *runner, PointerArray *array, const char *cmd) NONNULL_ARGS WARN_UNUSED_RESULT;
const char *command_parse_error_to_string(CommandParseError err) RETURNS_NONNULL;

#endif
