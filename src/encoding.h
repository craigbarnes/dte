#ifndef ENCODING_H
#define ENCODING_H

#include "libc.h"

typedef struct {
    const char *encoding;
    unsigned char bytes[4];
    int len;
} ByteOrderMark;

char *normalize_encoding(const char *encoding);
const ByteOrderMark *get_bom_for_encoding(const char *encoding);
const char *detect_encoding_from_bom(const unsigned char *buf, size_t size);

#endif
