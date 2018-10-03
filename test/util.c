#include "test.h"
#include "../src/util/ascii.h"
#include "../src/util/string.h"
#include "../src/util/strtonum.h"
#include "../src/util/uchar.h"
#include "../src/util/unicode.h"

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

void test_util(void)
{
    test_ascii();
    test_string();
    test_number_width();
    test_u_char_width();
    test_u_to_lower();
    test_u_is_upper();
    test_u_str_width();
}
