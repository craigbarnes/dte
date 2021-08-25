#ifndef EDITORCONFIG_INI_H
#define EDITORCONFIG_INI_H

#include <stddef.h>
#include "util/string-view.h"

typedef struct {
    StringView section;
    StringView name;
    StringView value;
    unsigned int name_idx;
} IniData;

typedef int (*IniCallback)(const IniData *data, void *userdata);

void ini_parse(const char *buf, size_t size, IniCallback callback, void *userdata);

#endif
