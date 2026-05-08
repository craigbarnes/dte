#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "buffer.h"
#include "command/error.h"
#include "options.h"
#include "util/macros.h"
#include "util/numtostr.h"

typedef struct {
    ErrorBuffer *ebuf;
    const char *encoding;
    mode_t new_file_mode;
    bool crlf;
    bool write_bom;
    bool hardlinks;
} FileSaveContext;

bool load_buffer(Buffer *buffer, const char *filename, const GlobalOptions *gopts, ErrorBuffer *ebuf, bool must_exist) NONNULL_ARG(1, 2, 3) WARN_UNUSED_RESULT;
bool save_buffer(Buffer *buffer, const char *filename, const FileSaveContext *ctx) NONNULL_ARGS WARN_UNUSED_RESULT;
bool read_blocks(Buffer *buffer, const GlobalOptions *gopts, ErrorBuffer *ebuf, int fd) NONNULL_ARG(1, 2) WARN_UNUSED_RESULT;

WARN_UNUSED_RESULT NONNULL_ARG(1, 3, 4, 5)
static inline bool size_exceeds_limit ( // NOLINT(readability-function-size)
    ErrorBuffer *ebuf,
    const char *filename,
    const char *size_name,
    const char *limit_name,
    const char *extra_msg,
    uintmax_t size,
    uintmax_t limit
) {
    if (likely(!limit || size <= limit)) {
        return false;
    }

    const char *sep = filename ? ": " : "";
    char limit_str[PRECISE_FILESIZE_STR_MAX];
    char size_str[HRSIZE_MAX];
    filesize_to_str_precise(limit, limit_str);
    human_readable_size(size, size_str);
    filename = filename ? filename : "";

    error_msg (
        ebuf,
        "%s size (%s) exceeds '%s' option (%s)%s%s%s",
        size_name, size_str, limit_name, limit_str, extra_msg, sep, filename
    );

    return true;
}

#endif
