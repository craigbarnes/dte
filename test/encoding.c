#include "test.h"
#include "../src/encoding/bom.h"
#include "../src/encoding/encoding.h"

static void test_detect_encoding_from_bom(void)
{
    static const struct bom_test {
        EncodingType encoding;
        const unsigned char *text;
        size_t size;
    } tests[] = {
        {UTF8, "\xef\xbb\xbfHello", 8},
        {UTF32BE, "\x00\x00\xfe\xffHello", 9},
        {UTF32LE, "\xff\xfe\x00\x00Hello", 9},
        {UTF16BE, "\xfe\xffHello", 7},
        {UTF16LE, "\xff\xfeHello", 7},
        {UNKNOWN_ENCODING, "\x00\xef\xbb\xbfHello", 9},
        {UNKNOWN_ENCODING, "\xef\xbb", 2},
    };
    FOR_EACH_I(i, tests) {
        const struct bom_test *t = &tests[i];
        EncodingType result = detect_encoding_from_bom(t->text, t->size);
        IEXPECT_EQ(result, t->encoding);
    }
}

static void test_lookup_encoding(void)
{
    static const struct lookup_encoding_test {
        EncodingType encoding;
        const char *name;
    } tests[] = {
        {UTF8, "UTF-8"},
        {UTF8, "UTF8"},
        {UTF8, "utf-8"},
        {UTF8, "utf8"},
        {UTF8, "Utf8"},
        {UTF16, "UTF16"},
        {UTF32LE, "utf-32le"},
        {UTF32LE, "ucs-4le"},
        {UNKNOWN_ENCODING, "UTF8_"},
        {UNKNOWN_ENCODING, "UTF"},
    };
    FOR_EACH_I(i, tests) {
        EncodingType result = lookup_encoding(tests[i].name);
        IEXPECT_EQ(result, tests[i].encoding);
    }
}

void test_encoding(void)
{
    test_detect_encoding_from_bom();
    test_lookup_encoding();
}
