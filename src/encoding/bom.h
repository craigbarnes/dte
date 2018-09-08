#ifndef ENCODING_BOM_H
#define ENCODING_BOM_H

#include <stddef.h>

typedef struct {
    const char *encoding;
    unsigned char bytes[4];
    size_t len;
} ByteOrderMark;

const ByteOrderMark *get_bom_for_encoding(const char *encoding);
const char *detect_encoding_from_bom(const unsigned char *buf, size_t size);

#endif
