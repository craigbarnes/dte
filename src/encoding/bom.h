#ifndef ENCODING_BOM_H
#define ENCODING_BOM_H

#include <stddef.h>
#include "encoding.h"

typedef struct {
    const unsigned char bytes[4];
    unsigned int len;
} ByteOrderMark;

const ByteOrderMark *get_bom_for_encoding(EncodingType encoding);
EncodingType detect_encoding_from_bom(const unsigned char *buf, size_t size);

#endif
