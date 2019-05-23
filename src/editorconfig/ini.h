#ifndef EDITORCONFIG_INI_H
#define EDITORCONFIG_INI_H

#include "../util/string-view.h"

typedef struct {
    StringView section;
    StringView name;
    StringView value;
    unsigned int name_idx;
} IniData;

typedef int (*IniCallback)(const IniData *data, void *userdata);

int ini_parse(const char *filename, IniCallback handler, void *userdata);

#endif
