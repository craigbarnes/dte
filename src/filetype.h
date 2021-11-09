#ifndef FILETYPE_H
#define FILETYPE_H

#include <stdbool.h>
#include <string.h>
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"

typedef enum {
    FT_EXTENSION,
    FT_FILENAME,
    FT_CONTENT,
    FT_INTERPRETER,
    FT_BASENAME,
} FileDetectionType;

static inline bool is_valid_filetype_name(const char *name)
{
    return name[0] != '\0' && !strchr(name, '/');
}

void add_filetype(PointerArray *filetypes, const char *name, const char *str, FileDetectionType type);
bool is_ft(const PointerArray *filetypes, const char *name);
const char *find_ft(const PointerArray *filetypes, const char *filename, StringView line);
void collect_ft(PointerArray *a, const char *prefix);
String dump_ft(void);

#endif
