#ifndef FILETYPE_H
#define FILETYPE_H

#include <stdbool.h>
#include <stddef.h>

enum detect_type {
    FT_EXTENSION,
    FT_FILENAME,
    FT_CONTENT,
    FT_INTERPRETER,
    FT_BASENAME,
};

void add_filetype(const char *name, const char *str, enum detect_type type);
bool is_ft(const char *name);

const char *find_ft (
    const char *filename,
    const char *interpreter,
    const char *first_line,
    size_t line_len
);

#endif
