#include "test.h"
#include "encoding.h"

static void test_detect_encoding_from_bom(TestContext *ctx)
{
    static const struct bom_test {
        EncodingType encoding;
        const char *text;
        size_t size;
    } tests[] = {
        {UTF8, STRN("\xef\xbb\xbfHello")},
        {UTF32BE, STRN("\x00\x00\xfe\xffHello")},
        {UTF32LE, STRN("\xff\xfe\x00\x00Hello")},
        {UTF16BE, STRN("\xfe\xffHello")},
        {UTF16LE, STRN("\xff\xfeHello")},
        {UTF16LE, STRN("\xff\xfe")},
        {UNKNOWN_ENCODING, STRN("\xff\xff\x00\x00Hello")},
        {UNKNOWN_ENCODING, STRN("\x00\xef\xbb\xbfHello")},
        {UNKNOWN_ENCODING, STRN("\xef\xbb")},
        {UNKNOWN_ENCODING, STRN("\xef\xbb#")},
        {UNKNOWN_ENCODING, STRN("\xee\xbb\xbf")},
        {UNKNOWN_ENCODING, STRN("\xff\xbb\xbf")},
        {UNKNOWN_ENCODING, STRN("\xbf\xbb\xef")},
        {UNKNOWN_ENCODING, STRN("\x00\x00\xfe")},
        {UNKNOWN_ENCODING, STRN("\x00")},
        {UNKNOWN_ENCODING, "", 0},
        {UNKNOWN_ENCODING, NULL, 0},
    };
    FOR_EACH_I(i, tests) {
        const struct bom_test *t = &tests[i];
        EncodingType result = detect_encoding_from_bom(t->text, t->size);
        IEXPECT_EQ(result, t->encoding);
    }
}

static void test_lookup_encoding(TestContext *ctx)
{
    static const struct {
        EncodingType encoding;
        const char *name;
    } tests[] = {
        {UTF8, "UTF-8"},
        {UTF8, "UTF8"},
        {UTF8, "utf-8"},
        {UTF8, "utf8"},
        {UTF8, "Utf8"},
        {UTF16BE, "UTF16"},
        {UTF16BE, "UTF-16"},
        {UTF32BE, "utf32"},
        {UTF32BE, "utf-32"},
        {UTF32LE, "utf-32le"},
        {UTF32LE, "ucs-4le"},
        {UTF32BE, "ucs-4BE"},
        {UTF32BE, "ucs-4"},
        {UTF32BE, "ucs4"},
        {UNKNOWN_ENCODING, "UTF8_"},
        {UNKNOWN_ENCODING, "UTF"},
    };
    FOR_EACH_I(i, tests) {
        EncodingType result = lookup_encoding(tests[i].name);
        IEXPECT_EQ(result, tests[i].encoding);
    }
}

static void test_encoding_from_type(TestContext *ctx)
{
    const char *a = encoding_from_type(UTF8);
    EXPECT_STREQ(a, "UTF-8");
    EXPECT_TRUE(encoding_is_utf8(a));
    // Ensure returned value is an "interned" string
    EXPECT_PTREQ(a, encoding_normalize("utf8"));
    EXPECT_PTREQ(a, encoding_from_type(UTF8));
}

static void test_get_bom_for_encoding(TestContext *ctx)
{
    const ByteOrderMark *bom = get_bom_for_encoding(UTF8);
    EXPECT_MEMEQ(bom->bytes, bom->len, "\xef\xbb\xbf", 3);

    bom = get_bom_for_encoding(UTF32LE);
    EXPECT_MEMEQ(bom->bytes, bom->len, "\xff\xfe\0\0", 4);

    bom = get_bom_for_encoding(UTF16BE);
    EXPECT_MEMEQ(bom->bytes, bom->len, "\xfe\xff", 2);

    EXPECT_NULL(get_bom_for_encoding(UNKNOWN_ENCODING));
    EXPECT_FALSE(encoding_type_has_bom(UNKNOWN_ENCODING));
    EXPECT_TRUE(encoding_type_has_bom(UTF8));
    EXPECT_TRUE(encoding_type_has_bom(UTF32LE));
}

static const TestEntry tests[] = {
    TEST(test_detect_encoding_from_bom),
    TEST(test_lookup_encoding),
    TEST(test_encoding_from_type),
    TEST(test_get_bom_for_encoding),
};

const TestGroup encoding_tests = TEST_GROUP(tests);
