#include "encoding.h"
#include "util/ascii.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/xstring.h"

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

static const ByteOrderMark boms[] = {
    [UTF8] = {{0xef, 0xbb, 0xbf}, 3},
    [UTF16BE] = {{0xfe, 0xff}, 2},
    [UTF16LE] = {{0xff, 0xfe}, 2},
    [UTF32BE] = {{0x00, 0x00, 0xfe, 0xff}, 4},
    [UTF32LE] = {{0xff, 0xfe, 0x00, 0x00}, 4},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY_ICASE(encoding_aliases, alias);
    CHECK_STRING_ARRAY(encoding_names);
    static_assert(ARRAYLEN(encoding_names) == UNKNOWN_ENCODING);
    static_assert(ARRAYLEN(boms) == UNKNOWN_ENCODING);
}

static int enc_alias_cmp(const void *key, const void *elem)
{
    const EncodingAlias *a = key;
    const char *name = elem;
    return ascii_strcmp_icase(a->alias, name);
}

EncodingType lookup_encoding(const char *name)
{
    if (likely(name == encoding_names[UTF8])) {
        return UTF8;
    }

    for (size_t i = 0; i < ARRAYLEN(encoding_names); i++) {
        if (ascii_streq_icase(name, encoding_names[i])) {
            return (EncodingType) i;
        }
    }

    const EncodingAlias *a = BSEARCH(name, encoding_aliases, enc_alias_cmp);
    return a ? a->encoding : UNKNOWN_ENCODING;
}

const char *encoding_from_type(EncodingType type)
{
    BUG_ON(type >= UNKNOWN_ENCODING);

    // There's no need to call str_intern() here; the names in the array
    // can be considered static interns
    return encoding_names[type];
}

const char *encoding_normalize(const char *name)
{
    EncodingType type = lookup_encoding(name);
    if (type != UNKNOWN_ENCODING) {
        return encoding_from_type(type);
    }

    char upper[256];
    size_t n;
    for (n = 0; n < sizeof(upper) && name[n]; n++) {
        upper[n] = ascii_toupper(name[n]);
    }

    return mem_intern(upper, n);
}

EncodingType detect_encoding_from_bom(const unsigned char *buf, size_t size)
{
    // Skip exhaustive checks if there's clearly no BOM
    if (size < 2 || ((unsigned int)buf[0]) - 1 < 0xEE) {
        return UNKNOWN_ENCODING;
    }

    // Iterate array backwards to ensure UTF32LE is checked before UTF16LE
    for (size_t n = ARRAYLEN(boms), i = n - 1; i < n; i--) {
        const unsigned int bom_len = boms[i].len;
        BUG_ON(bom_len == 0);
        if (size >= bom_len && mem_equal(buf, boms[i].bytes, bom_len)) {
            return (EncodingType)i;
        }
    }

    return UNKNOWN_ENCODING;
}

const ByteOrderMark *get_bom_for_encoding(EncodingType type)
{
    return encoding_type_has_bom(type) ? &boms[type] : NULL;
}
