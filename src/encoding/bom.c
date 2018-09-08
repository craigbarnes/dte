#include <string.h>
#include "bom.h"
#include "../util/macros.h"

static const ByteOrderMark boms[] = {
    {"UTF-8", {0xef, 0xbb, 0xbf}, 3},
    {"UTF-32BE", {0x00, 0x00, 0xfe, 0xff}, 4},
    {"UTF-32LE", {0xff, 0xfe, 0x00, 0x00}, 4},
    {"UTF-16BE", {0xfe, 0xff}, 2},
    {"UTF-16LE", {0xff, 0xfe}, 2},
};

static const ByteOrderMark *get_bom(const unsigned char *buf, size_t size)
{
    for (size_t i = 0; i < ARRAY_COUNT(boms); i++) {
        const ByteOrderMark *bom = &boms[i];
        if (size >= bom->len && !memcmp(buf, bom->bytes, bom->len)) {
            return bom;
        }
    }
    return NULL;
}

const ByteOrderMark *get_bom_for_encoding(const char *encoding)
{
    for (size_t i = 0; i < ARRAY_COUNT(boms); i++) {
        const ByteOrderMark *bom = &boms[i];
        if (strcmp(bom->encoding, encoding) == 0) {
            return bom;
        }
    }
    return NULL;
}

const char *detect_encoding_from_bom(const unsigned char *buf, size_t size)
{
    const ByteOrderMark *bom = get_bom(buf, size);
    if (bom) {
        return bom->encoding;
    }
    return NULL;
}
