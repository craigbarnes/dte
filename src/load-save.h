#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include <stdbool.h>
#include <sys/types.h>
#include "buffer.h"
#include "util/macros.h"

typedef struct {
    const char *encoding;
    mode_t new_file_mode;
    bool crlf;
    bool write_bom;
    bool hardlinks;
} FileSaveContext;

struct EditorState;

bool load_buffer(struct EditorState *e, Buffer *buffer, const char *filename, bool must_exist) NONNULL_ARGS WARN_UNUSED_RESULT;
bool save_buffer(struct EditorState *e, Buffer *buffer, const char *filename, const FileSaveContext *ctx) NONNULL_ARGS WARN_UNUSED_RESULT;
bool read_blocks(Buffer *buffer, int fd, bool utf8_bom) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
