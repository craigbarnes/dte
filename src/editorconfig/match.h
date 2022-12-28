#ifndef EDITORCONFIG_MATCH_H
#define EDITORCONFIG_MATCH_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"

bool ec_pattern_match(const char *pattern, size_t pat_len, const char *path) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
