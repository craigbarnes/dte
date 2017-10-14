#ifndef FILETYPE_H
#define FILETYPE_H

#include "libc.h"

enum detect_type {
    FT_EXTENSION,
    FT_FILENAME,
    FT_CONTENT,
    FT_INTERPRETER,
};

void add_filetype(const char *name, const char *str, enum detect_type type);
bool is_ft(const char *name);

const char *find_ft (
    const char *filename,
    const char *interpreter,
    const char *first_line,
    unsigned int line_len
);

#endif
