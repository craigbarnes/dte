#ifndef EDITORCONFIG_MATCH_H
#define EDITORCONFIG_MATCH_H

#include <stdbool.h>
#include "../util/macros.h"

bool ec_pattern_match(const char *pattern, const char *path) NONNULL_ARGS;

#endif
