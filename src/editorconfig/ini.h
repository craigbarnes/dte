#ifndef EDITORCONFIG_INI_H
#define EDITORCONFIG_INI_H

#include <stddef.h>
#include "util/macros.h"
#include "util/string-view.h"

typedef struct {
    const char *input;
    size_t input_len;
    size_t pos;
    StringView section;
    StringView name;
    StringView value;
    unsigned int name_count;
} IniParser;

bool ini_parse(IniParser *ctx) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
