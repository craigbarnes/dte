#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#include "encoding.h"
#include "../common.h"
#include "../util/ascii.h"
#include "../util/xmalloc.h"

typedef enum {
    UTF8,
    UTF16,
    UTF16BE,
    UTF16LE,
    UTF32,
    UTF32BE,
    UTF32LE,
    UNKNOWN_ENCODING,
} EncodingType;

static const char encoding_names[][16] = {
    [UTF8] = "UTF-8",
    [UTF16] = "UTF-16",
    [UTF16BE] = "UTF-16BE",
    [UTF16LE] = "UTF-16LE",
    [UTF32] = "UTF-32",
    [UTF32BE] = "UTF-32BE",
    [UTF32LE] = "UTF-32LE",
};

static_assert(UNKNOWN_ENCODING == ARRAY_COUNT(encoding_names));

static const struct {
    const char alias[8];
    EncodingType encoding;
} encoding_aliases[] = {
    {"UTF8", UTF8},
    {"UTF16", UTF16},
    {"UTF16BE", UTF16BE},
    {"UTF16LE", UTF16LE},
    {"UTF32", UTF32},
    {"UTF32BE", UTF32BE},
    {"UTF32LE", UTF32LE},
    {"UCS2", UTF16},
    {"UCS-2", UTF16},
    {"UCS-2BE", UTF16BE},
    {"UCS-2LE", UTF16LE},
    {"UCS4", UTF32},
    {"UCS-4", UTF32},
    {"UCS-4BE", UTF32BE},
    {"UCS-4LE", UTF32LE},
};

static const ByteOrderMark boms[] = {
    {"UTF-8", {0xef, 0xbb, 0xbf}, 3},
    {"UTF-32BE", {0x00, 0x00, 0xfe, 0xff}, 4},
    {"UTF-32LE", {0xff, 0xfe, 0x00, 0x00}, 4},
    {"UTF-16BE", {0xfe, 0xff}, 2},
    {"UTF-16LE", {0xff, 0xfe}, 2},
};

static bool encoding_is_supported_by_iconv(const char *encoding)
{
    iconv_t cd = iconv_open("UTF-8", encoding);
    if (cd == (iconv_t) -1) {
        return false;
    }
    iconv_close(cd);
    return true;
}

static EncodingType lookup_encoding(const char *name)
{
    for (size_t i = 0; i < ARRAY_COUNT(encoding_names); i++) {
        if (strcasecmp(name, encoding_names[i]) == 0) {
            return (EncodingType) i;
        }
    }
    for (size_t i = 0; i < ARRAY_COUNT(encoding_aliases); i++) {
        if (strcasecmp(name, encoding_aliases[i].alias) == 0) {
            return encoding_aliases[i].encoding;
        }
    }
    return UNKNOWN_ENCODING;
}

UNITTEST {
    BUG_ON(lookup_encoding("UTF-8") != UTF8);
    BUG_ON(lookup_encoding("UTF8") != UTF8);
    BUG_ON(lookup_encoding("utf-8") != UTF8);
    BUG_ON(lookup_encoding("utf8") != UTF8);
    BUG_ON(lookup_encoding("Utf8") != UTF8);
    BUG_ON(lookup_encoding("UTF16") != UTF16);
    BUG_ON(lookup_encoding("utf-32le") != UTF32LE);
    BUG_ON(lookup_encoding("ucs-4le") != UTF32LE);
    BUG_ON(lookup_encoding("UTF8_") != UNKNOWN_ENCODING);
    BUG_ON(lookup_encoding("UTF") != UNKNOWN_ENCODING);
}

char *normalize_encoding(const char *encoding)
{
    EncodingType type = lookup_encoding(encoding);
    if (type == UTF8) {
        return xstrdup(encoding_names[UTF8]);
    }

    char *normalized;
    if (type == UNKNOWN_ENCODING) {
        normalized = xstrdup(encoding);
        for (size_t i = 0; normalized[i]; i++) {
            normalized[i] = ascii_toupper(normalized[i]);
        }
    } else {
        normalized = xstrdup(encoding_names[type]);
    }

    if (encoding_is_supported_by_iconv(normalized)) {
        return normalized;
    }

    free(normalized);
    return NULL;
}

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
        if (streq(bom->encoding, encoding)) {
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
