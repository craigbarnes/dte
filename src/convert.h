#ifndef CONVERT_H
#define CONVERT_H

#include <stdbool.h>
#include <sys/types.h>
#include "buffer.h"
#include "util/macros.h"

typedef struct {
    struct cconv *cconv;
    unsigned char *nbuf;
    size_t nsize;
    bool crlf;
    int fd;
} FileEncoder;

bool conversion_supported_by_iconv(const char *from, const char *to) NONNULL_ARGS WARN_UNUSED_RESULT;
bool file_decoder_read(Buffer *buffer, const unsigned char *buf, size_t size) NONNULL_ARGS WARN_UNUSED_RESULT;

FileEncoder file_encoder(const char *encoding, bool crlf, int fd) NONNULL_ARGS WARN_UNUSED_RESULT;
void file_encoder_free(FileEncoder *enc) NONNULL_ARGS;
ssize_t file_encoder_write(FileEncoder *enc, const unsigned char *buf, size_t size) NONNULL_ARGS WARN_UNUSED_RESULT;
size_t file_encoder_get_nr_errors(const FileEncoder *enc) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
