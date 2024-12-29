#ifndef FILE_OPTION_H
#define FILE_OPTION_H

#include <stdbool.h>
#include "buffer.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"

typedef enum {
    FOPTS_FILENAME,
    FOPTS_FILETYPE,
} FileOptionType;

typedef union {
    const char *filetype; // Interned
    const InternedRegexp *filename;
} FileTypeOrFileName;

struct EditorState;

void set_file_options(struct EditorState *e, Buffer *buffer) NONNULL_ARGS;
void set_editorconfig_options(Buffer *buffer) NONNULL_ARGS;
void dump_file_options(const PointerArray *file_options, String *buf);
void free_file_options(PointerArray *file_options);

NONNULL_ARGS
void add_file_options (
    PointerArray *file_options,
    FileOptionType type,
    FileTypeOrFileName u,
    char **strs,
    size_t nstrs
);

#endif
