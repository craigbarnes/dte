#ifndef COMMAND_PARSE_H
#define COMMAND_PARSE_H

#include <stdbool.h>
#include <stddef.h>
#include "util/ptr-array.h"

typedef enum {
    CMDERR_NONE,
    CMDERR_UNCLOSED_SQUOTE,
    CMDERR_UNCLOSED_DQUOTE,
    CMDERR_UNEXPECTED_EOF,
} CommandParseError;

char *parse_command_arg(const char *cmd, size_t len, bool tilde);
size_t find_end(const char *cmd, size_t pos, CommandParseError *err);
CommandParseError parse_commands(PointerArray *array, const char *cmd);
const char *command_parse_error_to_string(CommandParseError err);

#endif
