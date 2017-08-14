#ifndef FILE_OPTION_H
#define FILE_OPTION_H

#include "buffer.h"

enum file_options_type {
    FILE_OPTIONS_FILENAME,
    FILE_OPTIONS_FILETYPE,
};

void set_file_options(Buffer *b);
void add_file_options(enum file_options_type type, char *to, char **strs);

#endif
