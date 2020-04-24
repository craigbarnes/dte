#ifndef EDITORCONFIG_MATCH_H
#define EDITORCONFIG_MATCH_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"

NONNULL_ARGS
bool ec_pattern_match(const char *pattern, size_t pat_len, const char *path);

#endif
