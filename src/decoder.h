#ifndef DECODER_H
#define DECODER_H

#include <sys/types.h>
#include <stdbool.h>

typedef struct FileDecoder {
    char *encoding;
    const unsigned char *ibuf;
    ssize_t ipos, isize;
    struct cconv *cconv;
    bool (*read_line)(struct FileDecoder *dec, char **linep, ssize_t *lenp);
} FileDecoder;

FileDecoder *new_file_decoder(const char *encoding, const unsigned char *buf, ssize_t size);
void free_file_decoder(FileDecoder *dec);
bool file_decoder_read_line(FileDecoder *dec, char **line, ssize_t *len);

#endif
