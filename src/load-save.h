#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include <stdbool.h>
#include <sys/types.h>
#include "buffer.h"
#include "encoding.h"
#include "options.h"
#include "util/macros.h"

typedef struct {
    const Encoding *encoding;
    mode_t new_file_mode;
    bool crlf;
    bool write_bom;
    bool hardlinks;
} FileSaveContext;

bool load_buffer(Buffer *buffer, const char *filename, const GlobalOptions *gopts, bool must_exist) NONNULL_ARGS WARN_UNUSED_RESULT;
bool save_buffer(Buffer *buffer, const char *filename, const FileSaveContext *ctx) NONNULL_ARGS WARN_UNUSED_RESULT;
bool read_blocks(Buffer *buffer, int fd, bool utf8_bom) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
