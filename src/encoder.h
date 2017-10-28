#ifndef ENCODER_H
#define ENCODER_H

#include <sys/types.h>
#include "options.h"

typedef struct {
    struct cconv *cconv;
    unsigned char *nbuf;
    ssize_t nsize;
    LineEndingType nls;
    int fd;
} FileEncoder;

FileEncoder *new_file_encoder(const char *encoding, LineEndingType nls, int fd);
void free_file_encoder(FileEncoder *enc);
ssize_t file_encoder_write(FileEncoder *enc, const unsigned char *buf, ssize_t size);

#endif
