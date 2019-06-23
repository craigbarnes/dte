#ifndef FILE_OPTION_H
#define FILE_OPTION_H

#include "buffer.h"

typedef enum {
    FILE_OPTIONS_FILENAME,
    FILE_OPTIONS_FILETYPE,
} FileOptionType;

void set_file_options(Buffer *b);
void add_file_options(FileOptionType type, char *to, char **strs);

void set_editorconfig_options(Buffer *b);

#endif
