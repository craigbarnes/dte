#ifndef FILETYPE_H
#define FILETYPE_H

#include <stdbool.h>
#include <string.h>
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

PURE
static inline bool is_valid_filetype_name(const char *name)
{
    size_t n = strcspn(name, " \t/");
    return n > 0 && n < 64 && name[n] == '\0' && name[0] != '-';
}

bool add_filetype(PointerArray *filetypes, const char *name, const char *str, FileDetectionType type) NONNULL_ARGS WARN_UNUSED_RESULT;
bool is_ft(const PointerArray *filetypes, const char *name);
const char *find_ft(const PointerArray *filetypes, const char *filename, StringView line);
void collect_ft(const PointerArray *filetypes, PointerArray *a, const char *prefix);
String dump_filetypes(const PointerArray *filetypes);
void free_filetypes(PointerArray *filetypes);

#endif
