#ifndef FILETYPE_H
#define FILETYPE_H

#include <stdbool.h>
#include <stddef.h>
#include "util/string-view.h"

typedef enum {
    FT_EXTENSION,
    FT_FILENAME,
    FT_CONTENT,
    FT_INTERPRETER,
    FT_BASENAME,
} FileDetectionType;

void add_filetype(const char *name, const char *str, FileDetectionType type);
bool is_ft(const char *name);
const char *find_ft(const char *filename, StringView line);

#endif
