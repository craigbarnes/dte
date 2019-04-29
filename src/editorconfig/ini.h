#ifndef EDITORCONFIG_INI_H
#define EDITORCONFIG_INI_H

typedef int (*IniCallback) (
    void *userdata,
    const char *section,
    const char *name,
    const char *value,
    unsigned int name_idx
);

int ini_parse(const char *filename, IniCallback handler, void *userdata);

#endif
