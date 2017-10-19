#include "encoding.h"
#include "common.h"

static const struct {
    const char *const encoding;
    const char *const alias;
} aliases[] = {
    {"UTF-8", "UTF8"},
    {"UTF-16", "UTF16"},
    {"UTF-16BE", "UTF16BE"},
    {"UTF-16LE", "UTF16LE"},
    {"UTF-32", "UTF32"},
    {"UTF-32BE", "UTF32BE"},
    {"UTF-32LE", "UTF32LE"},
    {"UTF-16", "UCS2"},
    {"UTF-16", "UCS-2"},
    {"UTF-16BE", "UCS-2BE"},
    {"UTF-16LE", "UCS-2LE"},
    {"UTF-16", "UCS4"},
    {"UTF-16", "UCS-4"},
    {"UTF-16BE", "UCS-4BE"},
    {"UTF-16LE", "UCS-4LE"},
};

static const ByteOrderMark boms[] = {
    {"UTF-8", {0xef, 0xbb, 0xbf}, 3},
    {"UTF-32BE", {0x00, 0x00, 0xfe, 0xff}, 4},
    {"UTF-32LE", {0xff, 0xfe, 0x00, 0x00}, 4},
    {"UTF-16BE", {0xfe, 0xff}, 2},
    {"UTF-16LE", {0xff, 0xfe}, 2},
};

char *normalize_encoding(const char *encoding)
{
    char *e = xstrdup(encoding);
    iconv_t cd;

    for (size_t i = 0; e[i]; i++) {
        e[i] = ascii_toupper(e[i]);
    }

    for (size_t i = 0; i < ARRAY_COUNT(aliases); i++) {
        if (streq(e, aliases[i].alias)) {
            free(e);
            e = xstrdup(aliases[i].encoding);
            break;
        }
    }

    if (streq(e, "UTF-8")) {
        return e;
    }

    cd = iconv_open("UTF-8", e);
    if (cd == (iconv_t)-1) {
        free(e);
        return NULL;
    }
    iconv_close(cd);
    return e;
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
