#include <string.h>
#include "bom.h"
#include "../debug.h"
#include "../util/macros.h"

static const ByteOrderMark boms[NR_ENCODING_TYPES] = {
    [UTF8] = {{0xef, 0xbb, 0xbf}, 3},
    [UTF16BE] = {{0xfe, 0xff}, 2},
    [UTF16LE] = {{0xff, 0xfe}, 2},
    [UTF32BE] = {{0x00, 0x00, 0xfe, 0xff}, 4},
    [UTF32LE] = {{0xff, 0xfe, 0x00, 0x00}, 4},
};

EncodingType detect_encoding_from_bom(const unsigned char *buf, size_t size)
{
    // Iterate array backwards to ensure UTF32LE is checked before UTF16LE
    for (int i = NR_ENCODING_TYPES - 1; i >= 0; i--) {
        const unsigned int bom_len = boms[i].len;
        if (bom_len > 0 && size >= bom_len && !memcmp(buf, boms[i].bytes, bom_len)) {
            return (EncodingType) i;
        }
    }
    return UNKNOWN_ENCODING;
}

const ByteOrderMark *get_bom_for_encoding(EncodingType encoding)
{
    BUG_ON(encoding >= NR_ENCODING_TYPES);
    const ByteOrderMark *bom = &boms[encoding];
    return bom->len ? bom : NULL;
}
