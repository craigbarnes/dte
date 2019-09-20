#ifndef ENCODING_ENCODER_H
#define ENCODING_ENCODER_H

#include <sys/types.h>
#include "../util/macros.h"
#include "../encoding/encoding.h"

typedef enum {
    NEWLINE_UNIX,
    NEWLINE_DOS,
} LineEndingType;

typedef struct {
    struct cconv *cconv;
    unsigned char *nbuf;
    size_t nsize;
    LineEndingType nls;
    int fd;
} FileEncoder;

FileEncoder *new_file_encoder(const Encoding *encoding, LineEndingType nls, int fd) NONNULL_ARGS;
void free_file_encoder(FileEncoder *enc);
ssize_t file_encoder_write(FileEncoder *enc, const unsigned char *buf, size_t size) NONNULL_ARGS;

#endif
