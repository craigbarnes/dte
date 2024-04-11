#ifndef FILETYPE_H
#define FILETYPE_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"

// Note: the order of these values changes the order of iteration
// in find_ft()
typedef enum {
    FT_INTERPRETER,
    FT_BASENAME,
    FT_CONTENT,
    FT_EXTENSION,
    FT_FILENAME,
} FileDetectionType;

enum {
    FILETYPE_NAME_MAX = 63,
};

bool is_valid_filetype_name_sv(const StringView *name) PURE NONNULL_ARGS;

static inline bool is_valid_filetype_name(const char *name)
{
    const StringView sv = strview_from_cstring(name);
    return is_valid_filetype_name_sv(&sv);
}

bool add_filetype(PointerArray *filetypes, const char *name, const char *str, FileDetectionType type) NONNULL_ARGS WARN_UNUSED_RESULT;
bool is_ft(const PointerArray *filetypes, const char *name);
const char *find_ft(const PointerArray *filetypes, const char *filename, StringView line);
void collect_ft(const PointerArray *filetypes, PointerArray *a, const char *prefix);
String dump_filetypes(const PointerArray *filetypes);
void free_filetypes(PointerArray *filetypes);

#endif
