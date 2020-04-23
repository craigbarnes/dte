#ifndef FILE_OPTION_H
#define FILE_OPTION_H

#include "buffer.h"
#include "util/macros.h"
#include "util/string.h"

typedef enum {
    FILE_OPTIONS_FILENAME,
    FILE_OPTIONS_FILETYPE,
} FileOptionType;

void add_file_options(FileOptionType type, char *to, char **strs) NONNULL_ARGS;
void set_file_options(Buffer *b) NONNULL_ARGS;
void set_editorconfig_options(Buffer *b) NONNULL_ARGS;
void dump_file_options(String *buf);

#endif
