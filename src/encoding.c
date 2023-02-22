#include "encoding.h"
#include "util/ascii.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/str-util.h"

typedef struct {
    const char alias[8];
    EncodingType encoding;
} EncodingAlias;

static const char encoding_names[][16] = {
    [UTF8] = "UTF-8",
    [UTF16BE] = "UTF-16BE",
    [UTF16LE] = "UTF-16LE",
    [UTF32BE] = "UTF-32BE",
    [UTF32LE] = "UTF-32LE",
};

static const EncodingAlias encoding_aliases[] = {
    {"UCS-2", UTF16BE},
    {"UCS-2BE", UTF16BE},
    {"UCS-2LE", UTF16LE},
    {"UCS-4", UTF32BE},
    {"UCS-4BE", UTF32BE},
    {"UCS-4LE", UTF32LE},
    {"UCS2", UTF16BE},
    {"UCS4", UTF32BE},
    {"UTF-16", UTF16BE},
    {"UTF-32", UTF32BE},
    {"UTF16", UTF16BE},
    {"UTF16BE", UTF16BE},
    {"UTF16LE", UTF16LE},
    {"UTF32", UTF32BE},
    {"UTF32BE", UTF32BE},
    {"UTF32LE", UTF32LE},
    {"UTF8", UTF8},
};

static const ByteOrderMark boms[NR_ENCODING_TYPES] = {
    [UTF8] = {{0xef, 0xbb, 0xbf}, 3},
    [UTF16BE] = {{0xfe, 0xff}, 2},
    [UTF16LE] = {{0xff, 0xfe}, 2},
    [UTF32BE] = {{0x00, 0x00, 0xfe, 0xff}, 4},
    [UTF32LE] = {{0xff, 0xfe, 0x00, 0x00}, 4},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(encoding_aliases, alias, ascii_strcmp_icase);
}

static int enc_alias_cmp(const void *key, const void *elem)
{
    const EncodingAlias *a = key;
    const char *name = elem;
    return ascii_strcmp_icase(a->alias, name);
}

EncodingType lookup_encoding(const char *name)
{
    static_assert(ARRAYLEN(encoding_names) == NR_ENCODING_TYPES - 1);
    for (size_t i = 0; i < ARRAYLEN(encoding_names); i++) {
        if (ascii_streq_icase(name, encoding_names[i])) {
            return (EncodingType) i;
        }
    }

    const EncodingAlias *a = BSEARCH(name, encoding_aliases, enc_alias_cmp);
    return a ? a->encoding : UNKNOWN_ENCODING;
}

static const char *encoding_type_to_string(EncodingType type)
{
    if (type < NR_ENCODING_TYPES && type != UNKNOWN_ENCODING) {
        return str_intern(encoding_names[type]);
    }
    return NULL;
}

Encoding encoding_from_name(const char *name)
{
    const EncodingType type = lookup_encoding(name);
    const char *normalized_name;
    if (type == UNKNOWN_ENCODING) {
        char upper[256];
        size_t n;
        for (n = 0; n < sizeof(upper) && name[n]; n++) {
            upper[n] = ascii_toupper(name[n]);
        }
        normalized_name = mem_intern(upper, n);
    } else {
        normalized_name = encoding_type_to_string(type);
    }
    return (Encoding) {
        .type = type,
        .name = normalized_name
    };
}

Encoding encoding_from_type(EncodingType type)
{
    return (Encoding) {
        .type = type,
        .name = encoding_type_to_string(type)
    };
}

EncodingType detect_encoding_from_bom(const unsigned char *buf, size_t size)
{
    // Skip exhaustive checks if there's clearly no BOM
    if (size < 2 || ((unsigned int)buf[0]) - 1 < 0xEE) {
        return UNKNOWN_ENCODING;
    }

    // Iterate array backwards to ensure UTF32LE is checked before UTF16LE
    for (int i = NR_ENCODING_TYPES - 1; i >= 0; i--) {
        const unsigned int bom_len = boms[i].len;
        if (bom_len > 0 && size >= bom_len && mem_equal(buf, boms[i].bytes, bom_len)) {
            return (EncodingType) i;
        }
    }
    return UNKNOWN_ENCODING;
}

const ByteOrderMark *get_bom_for_encoding(EncodingType encoding)
{
    static_assert(ARRAYLEN(boms) == NR_ENCODING_TYPES);
    BUG_ON(encoding >= ARRAYLEN(boms));
    const ByteOrderMark *bom = &boms[encoding];
    return bom->len ? bom : NULL;
}
