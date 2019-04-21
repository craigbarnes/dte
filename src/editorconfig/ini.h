#ifndef EDITORCONFIG_INI_H
#define EDITORCONFIG_INI_H

#include <stdio.h>

#define MAX_SECTION_NAME 4096
#define MAX_PROPERTY_NAME 50
#define MAX_PROPERTY_VALUE 255

typedef int (*IniCallback) (
    void *user,
    const char *section,
    const char *name,
    const char *value,
    unsigned int name_idx
);

// For each name=value pair parsed, call handler function with given user
// pointer as well as section, name, and value (data only valid for duration
// of handler call). Handler should return nonzero on success, zero on error.
//
// Returns 0 on success, line number of first error on parse error (doesn't
// stop on first error), or -1 on file open error.
int ini_parse(const char *filename, IniCallback handler, void *user);

// Same as ini_parse(), but takes a FILE* instead of filename. This doesn't
// close the file when it's finished -- the caller must do that.
int ini_parse_file(FILE *file, IniCallback handler, void *user);

#endif
