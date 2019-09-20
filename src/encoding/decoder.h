#ifndef ENCODING_DECODER_H
#define ENCODING_DECODER_H

#include <stdbool.h>
#include <sys/types.h>

typedef struct FileDecoder {
    const char *encoding;
    const unsigned char *ibuf;
    ssize_t ipos, isize;
    struct cconv *cconv;
    bool (*read_line)(struct FileDecoder *dec, char **linep, size_t *lenp);
} FileDecoder;

FileDecoder *new_file_decoder(const char *encoding, const unsigned char *buf, size_t size);
void free_file_decoder(FileDecoder *dec);
bool file_decoder_read_line(FileDecoder *dec, char **line, size_t *len);

#endif
