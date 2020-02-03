#ifndef ENCODING_ENCODER_H
#define ENCODING_ENCODER_H

#include <stdbool.h>
#include <sys/types.h>
#include "../util/macros.h"
#include "../encoding/encoding.h"

typedef struct {
    struct cconv *cconv;
    unsigned char *nbuf;
    size_t nsize;
    bool crlf;
    int fd;
} FileEncoder;

FileEncoder *new_file_encoder(const Encoding *encoding, bool crlf, int fd) NONNULL_ARGS;
void free_file_encoder(FileEncoder *enc);
ssize_t file_encoder_write(FileEncoder *enc, const unsigned char *buf, size_t size) NONNULL_ARGS;

#endif
