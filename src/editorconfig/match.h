#ifndef EDITORCONFIG_MATCH_H
#define EDITORCONFIG_MATCH_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/string-view.h"
#include "util/string.h"

String ec_pattern_to_regex(StringView section, StringView dir) WARN_UNUSED_RESULT;
bool ec_pattern_match(StringView section, StringView dir, const char *path) WARN_UNUSED_RESULT NONNULL_ARGS;

#endif
