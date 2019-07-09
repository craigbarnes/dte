#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include <stdbool.h>
#include "command.h"
#include "util/macros.h"

bool parse_args(const Command *cmd, CommandArgs *a) NONNULL_ARGS;

#endif
