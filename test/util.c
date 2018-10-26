#include "test.h"
#include "../src/util/ascii.h"
#include "../src/util/hashset.h"
#include "../src/util/string.h"
#include "../src/util/strtonum.h"
#include "../src/util/uchar.h"
#include "../src/util/unicode.h"
#include "../src/util/xmalloc.h"

static void test_ascii(void)
{
    EXPECT_EQ(ascii_tolower('A'), 'a');
    EXPECT_EQ(ascii_tolower('Z'), 'z');
    EXPECT_EQ(ascii_tolower('a'), 'a');
    EXPECT_EQ(ascii_tolower('z'), 'z');
    EXPECT_EQ(ascii_tolower('9'), '9');
    EXPECT_EQ(ascii_tolower('~'), '~');
    EXPECT_EQ(ascii_tolower('\0'), '\0');

    EXPECT_EQ(ascii_toupper('a'), 'A');
    EXPECT_EQ(ascii_toupper('z'), 'Z');
    EXPECT_EQ(ascii_toupper('A'), 'A');
    EXPECT_EQ(ascii_toupper('Z'), 'Z');
    EXPECT_EQ(ascii_toupper('9'), '9');
    EXPECT_EQ(ascii_toupper('~'), '~');
    EXPECT_EQ(ascii_toupper('\0'), '\0');

    EXPECT_TRUE(ascii_isspace(' '));
    EXPECT_TRUE(ascii_isspace('\t'));
    EXPECT_TRUE(ascii_isspace('\r'));
    EXPECT_TRUE(ascii_isspace('\n'));

    EXPECT_TRUE(is_word_byte('a'));
    EXPECT_TRUE(is_word_byte('z'));
    EXPECT_TRUE(is_word_byte('A'));
    EXPECT_TRUE(is_word_byte('Z'));
    EXPECT_TRUE(is_word_byte('0'));
    EXPECT_TRUE(is_word_byte('9'));
    EXPECT_TRUE(is_word_byte('_'));
    EXPECT_TRUE(is_word_byte(0x80));
    EXPECT_TRUE(is_word_byte(0xFF));
    EXPECT_FALSE(is_word_byte('-'));
    EXPECT_FALSE(is_word_byte('.'));
    EXPECT_FALSE(is_word_byte(0x7F));
    EXPECT_FALSE(is_word_byte(0x00));

    EXPECT_EQ(hex_decode('0'), 0);
    EXPECT_EQ(hex_decode('9'), 9);
    EXPECT_EQ(hex_decode('a'), 10);
    EXPECT_EQ(hex_decode('A'), 10);
    EXPECT_EQ(hex_decode('f'), 15);
    EXPECT_EQ(hex_decode('F'), 15);
    EXPECT_EQ(hex_decode('g'), -1);
    EXPECT_EQ(hex_decode('G'), -1);
    EXPECT_EQ(hex_decode(' '), -1);
    EXPECT_EQ(hex_decode('\0'), -1);
    EXPECT_EQ(hex_decode('~'), -1);
}

static void test_string(void)
{
    String s = STRING_INIT;
    char *cstr = string_cstring(&s);
    EXPECT_STREQ(cstr, "");
    free(cstr);

    string_insert_ch(&s, 0, 0x1F4AF);
    EXPECT_EQ(s.len, 4);
    EXPECT_EQ(memcmp(s.buffer, "\xF0\x9F\x92\xAF", s.len), 0);

    string_add_str(&s, "test");
    EXPECT_EQ(s.len, 8);
    EXPECT_EQ(memcmp(s.buffer, "\xF0\x9F\x92\xAFtest", s.len), 0);

    string_remove(&s, 0, 5);
    EXPECT_EQ(s.len, 3);
    EXPECT_EQ(memcmp(s.buffer, "est", s.len), 0);

    string_make_space(&s, 0, 1);
    EXPECT_EQ(s.len, 4);
    s.buffer[0] = 't';
    EXPECT_EQ(memcmp(s.buffer, "test", s.len), 0);

    string_clear(&s);
    EXPECT_EQ(s.len, 0);
    string_insert_ch(&s, 0, 0x0E01);
    EXPECT_EQ(s.len, 3);
    EXPECT_EQ(memcmp(s.buffer, "\xE0\xB8\x81", s.len), 0);

    string_clear(&s);
    string_sprintf(&s, "%d %s\n", 88, "test");
    EXPECT_EQ(s.len, 8);
    EXPECT_EQ(memcmp(s.buffer, "88 test\n", s.len), 0);

    string_free(&s);
}

static void test_number_width(void)
{
    EXPECT_EQ(number_width(0), 1);
    EXPECT_EQ(number_width(-1), 2);
    EXPECT_EQ(number_width(420), 3);
    EXPECT_EQ(number_width(2147483647), 10);
    EXPECT_EQ(number_width(-2147483647), 11);
}

static void test_str_to_long(void)
{
    long val = 0;
    EXPECT_TRUE(str_to_long("0", &val));
    EXPECT_EQ(val, 0);
    EXPECT_TRUE(str_to_long("00001", &val));
    EXPECT_EQ(val, 1);
    EXPECT_TRUE(str_to_long("-00001", &val));
    EXPECT_EQ(val, -1);
    EXPECT_TRUE(str_to_long("-8074", &val));
    EXPECT_EQ(val, -8074);
    EXPECT_TRUE(str_to_long("2147483647", &val));
    EXPECT_EQ(val, 2147483647L);
    EXPECT_TRUE(str_to_long("-2147483647", &val));
    EXPECT_EQ(val, -2147483647L);
}

static void test_u_char_width(void)
{
    // ASCII (1 column)
    EXPECT_EQ(u_char_width('a'), 1);
    EXPECT_EQ(u_char_width('z'), 1);
    EXPECT_EQ(u_char_width('A'), 1);
    EXPECT_EQ(u_char_width('Z'), 1);
    EXPECT_EQ(u_char_width(' '), 1);
    EXPECT_EQ(u_char_width('!'), 1);
    EXPECT_EQ(u_char_width('/'), 1);
    EXPECT_EQ(u_char_width('^'), 1);
    EXPECT_EQ(u_char_width('`'), 1);
    EXPECT_EQ(u_char_width('~'), 1);

    // Rendered in caret notation (2 columns)
    EXPECT_EQ(u_char_width('\0'), 2);
    EXPECT_EQ(u_char_width('\r'), 2);
    EXPECT_EQ(u_char_width(0x1F), 2);

    // Rendered as <xx> (4 columns)
    EXPECT_EQ(u_char_width(0x0080), 4);
    EXPECT_EQ(u_char_width(0xDFFF), 4);

    // Zero width (0 columns)
    EXPECT_EQ(u_char_width(0xAA31), 0);
    EXPECT_EQ(u_char_width(0xAA32), 0);

    // Double width (2 columns)
    EXPECT_EQ(u_char_width(0x30000), 2);
    EXPECT_EQ(u_char_width(0x3A009), 2);
    EXPECT_EQ(u_char_width(0x3FFFD), 2);
    EXPECT_EQ(u_char_width(0x2757), 2);
    EXPECT_EQ(u_char_width(0x312F), 2);
}

static void test_u_to_lower(void)
{
    EXPECT_EQ(u_to_lower('A'), 'a');
    EXPECT_EQ(u_to_lower('Z'), 'z');
    EXPECT_EQ(u_to_lower('a'), 'a');
    EXPECT_EQ(u_to_lower('0'), '0');
    EXPECT_EQ(u_to_lower('~'), '~');
    EXPECT_EQ(u_to_lower('@'), '@');
    EXPECT_EQ(u_to_lower('\0'), '\0');
}

static void test_u_is_upper(void)
{
    EXPECT_TRUE(u_is_upper('A'));
    EXPECT_TRUE(u_is_upper('Z'));
    EXPECT_FALSE(u_is_upper('a'));
    EXPECT_FALSE(u_is_upper('z'));
    EXPECT_FALSE(u_is_upper('0'));
    EXPECT_FALSE(u_is_upper('9'));
    EXPECT_FALSE(u_is_upper('@'));
    EXPECT_FALSE(u_is_upper('['));
    EXPECT_FALSE(u_is_upper('{'));
    EXPECT_FALSE(u_is_upper('\0'));
    EXPECT_FALSE(u_is_upper('\t'));
    EXPECT_FALSE(u_is_upper(' '));
    EXPECT_FALSE(u_is_upper(0x00E0));
    EXPECT_FALSE(u_is_upper(0x00E7));
    EXPECT_FALSE(u_is_upper(0x1D499));
    EXPECT_FALSE(u_is_upper(0x1F315));
    EXPECT_FALSE(u_is_upper(0x10ffff));
}

static void test_u_str_width(void)
{
    EXPECT_EQ(u_str_width("foo"), 3);
    EXPECT_EQ (
        7, u_str_width (
            "\xE0\xB8\x81\xE0\xB8\xB3\xE0\xB9\x81\xE0\xB8\x9E\xE0\xB8"
            "\x87\xE0\xB8\xA1\xE0\xB8\xB5\xE0\xB8\xAB\xE0\xB8\xB9"
        )
    );
}

static void test_hashset(void)
{
    static const char *const strings[] = {
        "foo", "Foo", "bar", "quux", "etc",
        "\t\xff\x80\b", "\t\t\t", "\x01\x02\x03\xfe\xff",
#if __STDC_VERSION__ >= 201112L
        u8"ภาษาไทย",
        u8"中文",
        u8"日本語",
#endif
    };

    HashSet set;
    hashset_init(&set, (char**)strings, ARRAY_COUNT(strings), false);

    EXPECT_TRUE(hashset_contains(&set, "\t\xff\x80\b", 4));
    EXPECT_TRUE(hashset_contains(&set, "foo", 3));
    EXPECT_TRUE(hashset_contains(&set, "Foo", 3));

    EXPECT_FALSE(hashset_contains(&set, "FOO", 3));
    EXPECT_FALSE(hashset_contains(&set, "", 0));
    EXPECT_FALSE(hashset_contains(&set, NULL, 0));
    EXPECT_FALSE(hashset_contains(&set, "\0", 1));

    const char *last_string = strings[ARRAY_COUNT(strings) - 1];
    EXPECT_TRUE(hashset_contains(&set, last_string, strlen(last_string)));

    FOR_EACH_I(i, strings) {
        const char *str = strings[i];
        const size_t len = strlen(str);
        EXPECT_TRUE(hashset_contains(&set, str, len));
        EXPECT_FALSE(hashset_contains(&set, str, len - 1));
        EXPECT_FALSE(hashset_contains(&set, str + 1, len - 1));
    }

    hashset_free(&set);
    memzero(&set);

    hashset_init(&set, (char**)strings, ARRAY_COUNT(strings), true);
    EXPECT_TRUE(hashset_contains(&set, "foo", 3));
    EXPECT_TRUE(hashset_contains(&set, "FOO", 3));
    EXPECT_TRUE(hashset_contains(&set, "fOO", 3));
    hashset_free(&set);
}

static void test_round_up(void)
{
    EXPECT_EQ(ROUND_UP(3, 8), 8);
    EXPECT_EQ(ROUND_UP(8, 8), 8);
    EXPECT_EQ(ROUND_UP(9, 8), 16);
    EXPECT_EQ(ROUND_UP(0, 16), 0);
    EXPECT_EQ(ROUND_UP(1, 16), 16);
    EXPECT_EQ(ROUND_UP(123, 16), 128);
    EXPECT_EQ(ROUND_UP(4, 64), 64);
    EXPECT_EQ(ROUND_UP(80, 64), 128);
    EXPECT_EQ(ROUND_UP(256, 256), 256);
    EXPECT_EQ(ROUND_UP(257, 256), 512);
    EXPECT_EQ(ROUND_UP(8000, 256), 8192);
}

void test_util(void)
{
    test_ascii();
    test_string();
    test_number_width();
    test_str_to_long();
    test_u_char_width();
    test_u_to_lower();
    test_u_is_upper();
    test_u_str_width();
    test_hashset();
    test_round_up();
}
