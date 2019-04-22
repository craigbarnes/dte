#include <limits.h>
#include "test.h"
#include "../src/common.h"
#include "../src/syntax/hashset.h"
#include "../src/util/ascii.h"
#include "../src/util/bit.h"
#include "../src/util/path.h"
#include "../src/util/regexp.h"
#include "../src/util/string.h"
#include "../src/util/string-view.h"
#include "../src/util/strtonum.h"
#include "../src/util/unicode.h"
#include "../src/util/utf8.h"
#include "../src/util/xmalloc.h"
#include "../src/util/xsnprintf.h"

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
    EXPECT_FALSE(ascii_isspace('\0'));
    EXPECT_FALSE(ascii_isspace('a'));
    EXPECT_FALSE(ascii_isspace(0x7F));
    EXPECT_FALSE(ascii_isspace(0x80));

    EXPECT_TRUE(ascii_iscntrl('\0'));
    EXPECT_TRUE(ascii_iscntrl('\a'));
    EXPECT_TRUE(ascii_iscntrl('\b'));
    EXPECT_TRUE(ascii_iscntrl('\t'));
    EXPECT_TRUE(ascii_iscntrl('\n'));
    EXPECT_TRUE(ascii_iscntrl('\v'));
    EXPECT_TRUE(ascii_iscntrl('\f'));
    EXPECT_TRUE(ascii_iscntrl('\r'));
    EXPECT_TRUE(ascii_iscntrl(0x0E));
    EXPECT_TRUE(ascii_iscntrl(0x1F));
    EXPECT_TRUE(ascii_iscntrl(0x7F));
    EXPECT_FALSE(ascii_iscntrl(' '));
    EXPECT_FALSE(ascii_iscntrl('a'));
    EXPECT_FALSE(ascii_iscntrl(0x7E));
    EXPECT_FALSE(ascii_iscntrl(0x80));
    EXPECT_FALSE(ascii_iscntrl(0xFF));

    EXPECT_TRUE(ascii_is_nonspace_cntrl('\0'));
    EXPECT_TRUE(ascii_is_nonspace_cntrl('\a'));
    EXPECT_TRUE(ascii_is_nonspace_cntrl('\b'));
    EXPECT_TRUE(ascii_is_nonspace_cntrl(0x0E));
    EXPECT_TRUE(ascii_is_nonspace_cntrl(0x1F));
    EXPECT_TRUE(ascii_is_nonspace_cntrl(0x7F));
    EXPECT_FALSE(ascii_is_nonspace_cntrl('\t'));
    EXPECT_FALSE(ascii_is_nonspace_cntrl('\n'));
    EXPECT_FALSE(ascii_is_nonspace_cntrl('\r'));
    EXPECT_FALSE(ascii_is_nonspace_cntrl(' '));
    EXPECT_FALSE(ascii_is_nonspace_cntrl('a'));
    EXPECT_FALSE(ascii_is_nonspace_cntrl(0x7E));
    EXPECT_FALSE(ascii_is_nonspace_cntrl(0x80));
    EXPECT_FALSE(ascii_is_nonspace_cntrl(0xFF));

    EXPECT_TRUE(ascii_isprint(' '));
    EXPECT_TRUE(ascii_isprint('!'));
    EXPECT_TRUE(ascii_isprint('/'));
    EXPECT_TRUE(ascii_isprint('a'));
    EXPECT_TRUE(ascii_isprint('z'));
    EXPECT_TRUE(ascii_isprint('0'));
    EXPECT_TRUE(ascii_isprint('_'));
    EXPECT_TRUE(ascii_isprint('~'));
    EXPECT_FALSE(ascii_isprint('\0'));
    EXPECT_FALSE(ascii_isprint('\t'));
    EXPECT_FALSE(ascii_isprint('\n'));
    EXPECT_FALSE(ascii_isprint('\r'));
    EXPECT_FALSE(ascii_isprint(0x1F));
    EXPECT_FALSE(ascii_isprint(0x7F));
    EXPECT_FALSE(ascii_isprint(0x80));
    EXPECT_FALSE(ascii_isprint(0xFF));

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
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    EXPECT_PTREQ(s.buffer, NULL);

    for (size_t i = 0; i < 40; i++) {
        string_add_byte(&s, 'a');
    }

    EXPECT_EQ(s.len, 40);
    cstr = string_steal_cstring(&s);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    EXPECT_PTREQ(s.buffer, NULL);
    EXPECT_STREQ(cstr, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    free(cstr);
}

static void test_string_view(void)
{
    const StringView sv1 = STRING_VIEW("testing");
    EXPECT_TRUE(string_view_equal_cstr(&sv1, "testing"));
    EXPECT_FALSE(string_view_equal_cstr(&sv1, "testin"));
    EXPECT_FALSE(string_view_equal_cstr(&sv1, "TESTING"));
    EXPECT_TRUE(string_view_has_literal_prefix(&sv1, "test"));
    EXPECT_TRUE(string_view_has_literal_prefix_icase(&sv1, "TEst"));
    EXPECT_FALSE(string_view_has_literal_prefix(&sv1, "TEst"));
    EXPECT_FALSE(string_view_has_literal_prefix_icase(&sv1, "TEst_"));

    const StringView sv2 = string_view(sv1.data, sv1.length);
    EXPECT_TRUE(string_view_equal(&sv1, &sv2));

    const StringView sv3 = STRING_VIEW("\0test\0 ...");
    EXPECT_TRUE(string_view_equal_strn(&sv3, "\0test\0 ...", 10));
    EXPECT_TRUE(string_view_equal_literal(&sv3, "\0test\0 ..."));
    EXPECT_TRUE(string_view_has_prefix(&sv3, "\0test", 5));
    EXPECT_TRUE(string_view_has_literal_prefix(&sv3, "\0test\0"));
    EXPECT_FALSE(string_view_equal_cstr(&sv3, "\0test\0 ..."));

    const StringView sv4 = sv3;
    EXPECT_TRUE(string_view_equal(&sv4, &sv3));

    const StringView sv5 = string_view_from_cstring("foobar");
    EXPECT_TRUE(string_view_equal_literal(&sv5, "foobar"));
    EXPECT_TRUE(string_view_has_literal_prefix(&sv5, "foo"));
    EXPECT_FALSE(string_view_equal_cstr(&sv5, "foo"));
}

static void test_number_width(void)
{
    EXPECT_EQ(number_width(0), 1);
    EXPECT_EQ(number_width(-1), 2);
    EXPECT_EQ(number_width(420), 3);
    EXPECT_EQ(number_width(2147483647), 10);
    EXPECT_EQ(number_width(-2147483647), 11);
}

static void test_buf_parse_ulong(void)
{
    {
        char ulong_max[64];
        size_t ulong_max_len = xsnprintf(ulong_max, 64, "%lu", ULONG_MAX);

        unsigned long val = 88;
        size_t digits = buf_parse_ulong(ulong_max, ulong_max_len, &val);
        EXPECT_EQ(digits, ulong_max_len);
        EXPECT_EQ(val, ULONG_MAX);

        val = 99;
        ulong_max[ulong_max_len++] = '1';
        digits = buf_parse_ulong(ulong_max, ulong_max_len, &val);
        EXPECT_EQ(digits, 0);
        EXPECT_EQ(val, 99);
    }

    unsigned long val;
    EXPECT_TRUE(buf_parse_ulong("0", 1, &val));
    EXPECT_EQ(val, 0);
    EXPECT_TRUE(buf_parse_ulong("9876", 4, &val));
    EXPECT_EQ(val, 9876);
}

static void test_str_to_int(void)
{
    int val = 0;
    EXPECT_TRUE(str_to_int("-1", &val));
    EXPECT_EQ(val, -1);
    EXPECT_TRUE(str_to_int("+1", &val));
    EXPECT_EQ(val, 1);
    EXPECT_TRUE(str_to_int("0", &val));
    EXPECT_EQ(val, 0);
    EXPECT_TRUE(str_to_int("1", &val));
    EXPECT_EQ(val, 1);
    EXPECT_TRUE(str_to_int("+00299", &val));
    EXPECT_EQ(val, 299);

    EXPECT_FALSE(str_to_int("", &val));
    EXPECT_FALSE(str_to_int("100x", &val));
    EXPECT_FALSE(str_to_int("+-100", &val));
    EXPECT_FALSE(str_to_int("99999999999999999999999999999999", &val));
}

static void test_str_to_size(void)
{
    size_t val = 0;
    EXPECT_TRUE(str_to_size("100", &val));
    EXPECT_EQ(val, 100);
    EXPECT_TRUE(str_to_size("0", &val));
    EXPECT_EQ(val, 0);
    EXPECT_TRUE(str_to_size("000000001003", &val));
    EXPECT_EQ(val, 1003);
    EXPECT_TRUE(str_to_size("29132", &val));
    EXPECT_EQ(val, 29132);

    EXPECT_FALSE(str_to_size("", &val));
    EXPECT_FALSE(str_to_size("100x", &val));
    EXPECT_FALSE(str_to_size("-100", &val));
    EXPECT_FALSE(str_to_size("99999999999999999999999999999999", &val));
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

    // Unprintable (rendered as <xx> -- 4 columns)
    EXPECT_EQ(u_char_width(0x0080), 4);
    EXPECT_EQ(u_char_width(0xDFFF), 4);

    // Zero width (0 columns)
    EXPECT_EQ(u_char_width(0xAA31), 0);
    EXPECT_EQ(u_char_width(0xAA32), 0);

    // Double width (2 columns)
    EXPECT_EQ(u_char_width(0x2757), 2);
    EXPECT_EQ(u_char_width(0x312F), 2);

    // Double width but unassigned (rendered as <xx> -- 4 columns)
    EXPECT_EQ(u_char_width(0x30000), 4);
    EXPECT_EQ(u_char_width(0x3A009), 4);
    EXPECT_EQ(u_char_width(0x3FFFD), 4);

    // 1 column character >= 0x1100
    EXPECT_EQ(u_char_width(0x104B3), 1);
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

static void test_u_is_special_whitespace(void)
{
    EXPECT_FALSE(u_is_special_whitespace(' '));
    EXPECT_FALSE(u_is_special_whitespace('\t'));
    EXPECT_FALSE(u_is_special_whitespace('\n'));
    EXPECT_FALSE(u_is_special_whitespace('a'));
    EXPECT_FALSE(u_is_special_whitespace(0x1680));
    EXPECT_FALSE(u_is_special_whitespace(0x3000));
    EXPECT_TRUE(u_is_special_whitespace(0x00A0));
    EXPECT_TRUE(u_is_special_whitespace(0x00AD));
    EXPECT_TRUE(u_is_special_whitespace(0x2000));
    EXPECT_TRUE(u_is_special_whitespace(0x200a));
    EXPECT_TRUE(u_is_special_whitespace(0x2028));
    EXPECT_TRUE(u_is_special_whitespace(0x2029));
    EXPECT_TRUE(u_is_special_whitespace(0x202f));
    EXPECT_TRUE(u_is_special_whitespace(0x205f));
}

static void test_u_is_unprintable(void)
{
    // Private-use characters ------------------------------------------
    // (https://www.unicode.org/faq/private_use.html#pua2)

    // There are three ranges of private-use characters in the standard.
    // The main range in the BMP is U+E000..U+F8FF, containing 6,400
    // private-use characters.
    EXPECT_TRUE(u_is_unprintable(0xE000));
    EXPECT_TRUE(u_is_unprintable(0xF8FF));

    // ... there are also two large ranges of supplementary private-use
    // characters, consisting of most of the code points on planes 15
    // and 16: U+F0000..U+FFFFD and U+100000..U+10FFFD. Together those
    // ranges allocate another 131,068 private-use characters.
    EXPECT_TRUE(u_is_unprintable(0xF0000));
    EXPECT_TRUE(u_is_unprintable(0xFFFFD));
    EXPECT_TRUE(u_is_unprintable(0x100000));
    EXPECT_TRUE(u_is_unprintable(0x10FFFd));

    // Non-characters --------------------------------------------------
    // (https://www.unicode.org/faq/private_use.html#noncharacters)
    unsigned int noncharacter_count = 0;

    // "A contiguous range of 32 noncharacters: U+FDD0..U+FDEF in the BMP"
    for (CodePoint u = 0xFDD0; u <= 0xFDEF; u++) {
        EXPECT_TRUE(u_is_unprintable(u));
        noncharacter_count++;
    }

    // "The last two code points of the BMP, U+FFFE and U+FFFF"
    EXPECT_TRUE(u_is_unprintable(0xFFFE));
    EXPECT_TRUE(u_is_unprintable(0xFFFF));
    noncharacter_count += 2;

    // "The last two code points of each of the 16 supplementary planes:
    // U+1FFFE, U+1FFFF, U+2FFFE, U+2FFFF, ... U+10FFFE, U+10FFFF"
    for (CodePoint step = 1; step <= 16; step++) {
        const CodePoint u = (0x10000 * step) + 0xFFFE;
        EXPECT_TRUE(u_is_unprintable(u));
        EXPECT_TRUE(u_is_unprintable(u + 1));
        noncharacter_count += 2;
    }

    // "Q: How many noncharacters does Unicode have?"
    // "A: Exactly 66"
    EXPECT_EQ(noncharacter_count, 66);
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

static void test_u_set_char(void)
{
    char buf[16];
    size_t i;
    memzero(&buf);

    i = 0;
    u_set_char(buf, &i, 'a');
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], 'a');

    i = 0;
    u_set_char(buf, &i, 0x00DF);
    EXPECT_EQ(i, 2);
    EXPECT_EQ(memcmp(buf, "\xC3\x9F", 2), 0);

    i = 0;
    u_set_char(buf, &i, 0x0E01);
    EXPECT_EQ(i, 3);
    EXPECT_EQ(memcmp(buf, "\xE0\xB8\x81", 3), 0);

    i = 0;
    u_set_char(buf, &i, 0x1F914);
    EXPECT_EQ(i, 4);
    EXPECT_EQ(memcmp(buf, "\xF0\x9F\xA4\x94", 4), 0);

    i = 0;
    u_set_char(buf, &i, 0x10FFFF);
    EXPECT_EQ(i, 4);
    // Note: string separated to prevent "-Wtrigraphs" warning
    EXPECT_EQ(memcmp(buf, "<" "?" "?" ">", 4), 0);
}

static void test_u_set_ctrl(void)
{
    char buf[16];
    size_t i;
    memzero(&buf);

    i = 0;
    u_set_ctrl(buf, &i, '\0');
    EXPECT_EQ(i, 2);
    EXPECT_EQ(memcmp(buf, "^@", 3), 0);

    i = 0;
    u_set_ctrl(buf, &i, '\t');
    EXPECT_EQ(i, 2);
    EXPECT_EQ(memcmp(buf, "^I", 3), 0);

    i = 0;
    u_set_ctrl(buf, &i, 0x1F);
    EXPECT_EQ(i, 2);
    EXPECT_EQ(memcmp(buf, "^_", 3), 0);

    i = 0;
    u_set_ctrl(buf, &i, 0x7F);
    EXPECT_EQ(i, 2);
    EXPECT_EQ(memcmp(buf, "^?", 3), 0);
}

static void test_u_prev_char(void)
{
    const unsigned char *buf = "深圳市";
    size_t idx = 9;
    CodePoint c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, 0x5E02);
    EXPECT_EQ(idx, 6);
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, 0x5733);
    EXPECT_EQ(idx, 3);
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, 0x6DF1);
    EXPECT_EQ(idx, 0);

    idx = 1;
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, -((CodePoint)0xE6UL));
    EXPECT_EQ(idx, 0);

    buf = "Shenzhen";
    idx = 8;
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, 'n');
    EXPECT_EQ(idx, 7);
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, 'e');
    EXPECT_EQ(idx, 6);

    buf = "🥣🥤";
    idx = 8;
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, 0x1F964);
    EXPECT_EQ(idx, 4);
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, 0x1F963);
    EXPECT_EQ(idx, 0);

    buf = "\xF0\xF5";
    idx = 2;
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, -((CodePoint)buf[1]));
    EXPECT_EQ(idx, 1);
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, -((CodePoint)buf[0]));
    EXPECT_EQ(idx, 0);

    buf = "\xF5\xF0";
    idx = 2;
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, -((CodePoint)buf[1]));
    EXPECT_EQ(idx, 1);
    c = u_prev_char(buf, &idx);
    EXPECT_EQ(c, -((CodePoint)buf[0]));
    EXPECT_EQ(idx, 0);
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
    EXPECT_EQ(ROUND_UP(0, 8), 0);
    EXPECT_EQ(ROUND_UP(0, 16), 0);
    EXPECT_EQ(ROUND_UP(1, 16), 16);
    EXPECT_EQ(ROUND_UP(123, 16), 128);
    EXPECT_EQ(ROUND_UP(4, 64), 64);
    EXPECT_EQ(ROUND_UP(80, 64), 128);
    EXPECT_EQ(ROUND_UP(256, 256), 256);
    EXPECT_EQ(ROUND_UP(257, 256), 512);
    EXPECT_EQ(ROUND_UP(8000, 256), 8192);
}

void test_bitop(void)
{
    EXPECT_EQ(bit_popcount_u32(0), 0);
    EXPECT_EQ(bit_popcount_u32(1), 1);
    EXPECT_EQ(bit_popcount_u32(11), 3);
    EXPECT_EQ(bit_popcount_u32(128), 1);
    EXPECT_EQ(bit_popcount_u32(255), 8);
    EXPECT_EQ(bit_popcount_u32(UINT32_MAX), 32);
    EXPECT_EQ(bit_popcount_u32(UINT32_MAX - 1), 31);

    EXPECT_EQ(bit_popcount_u64(0), 0);
    EXPECT_EQ(bit_popcount_u64(1), 1);
    EXPECT_EQ(bit_popcount_u64(255), 8);
    EXPECT_EQ(bit_popcount_u64(UINT64_MAX), 64);
    EXPECT_EQ(bit_popcount_u64(UINT64_MAX - 1), 63);
    EXPECT_EQ(bit_popcount_u64(U64(0xFFFFFFFFFF)), 40);
    EXPECT_EQ(bit_popcount_u64(U64(0x10000000000)), 1);

    EXPECT_EQ(bit_count_leading_zeros_u64(1), 63);
    EXPECT_EQ(bit_count_leading_zeros_u64(4), 61);
    EXPECT_EQ(bit_count_leading_zeros_u64(127), 57);
    EXPECT_EQ(bit_count_leading_zeros_u64(128), 56);
    EXPECT_EQ(bit_count_leading_zeros_u64(UINT64_MAX), 0);
    EXPECT_EQ(bit_count_leading_zeros_u64(UINT64_MAX - 1), 0);

    EXPECT_EQ(bit_count_leading_zeros_u32(1), 31);
    EXPECT_EQ(bit_count_leading_zeros_u32(4), 29);
    EXPECT_EQ(bit_count_leading_zeros_u32(127), 25);
    EXPECT_EQ(bit_count_leading_zeros_u32(128), 24);
    EXPECT_EQ(bit_count_leading_zeros_u32(UINT32_MAX), 0);
    EXPECT_EQ(bit_count_leading_zeros_u32(UINT32_MAX - 1), 0);

    EXPECT_EQ(bit_count_trailing_zeros_u32(1), 0);
    EXPECT_EQ(bit_count_trailing_zeros_u32(2), 1);
    EXPECT_EQ(bit_count_trailing_zeros_u32(3), 0);
    EXPECT_EQ(bit_count_trailing_zeros_u32(4), 2);
    EXPECT_EQ(bit_count_trailing_zeros_u32(8), 3);
    EXPECT_EQ(bit_count_trailing_zeros_u32(13), 0);
    EXPECT_EQ(bit_count_trailing_zeros_u32(16), 4);
    EXPECT_EQ(bit_count_trailing_zeros_u32(U32(0xFFFFFFFE)), 1);
    EXPECT_EQ(bit_count_trailing_zeros_u32(U32(0x10000000)), 28);
    EXPECT_EQ(bit_count_trailing_zeros_u32(UINT32_MAX), 0);
    EXPECT_EQ(bit_count_trailing_zeros_u32(UINT32_MAX - 0xFF), 8);

    EXPECT_EQ(bit_count_trailing_zeros_u64(1), 0);
    EXPECT_EQ(bit_count_trailing_zeros_u64(2), 1);
    EXPECT_EQ(bit_count_trailing_zeros_u64(3), 0);
    EXPECT_EQ(bit_count_trailing_zeros_u64(16), 4);
    EXPECT_EQ(bit_count_trailing_zeros_u64(U64(0xFFFFFFFE)), 1);
    EXPECT_EQ(bit_count_trailing_zeros_u64(U64(0x10000000)), 28);
    EXPECT_EQ(bit_count_trailing_zeros_u64(U64(0x100000000000)), 44);
    EXPECT_EQ(bit_count_trailing_zeros_u64(UINT64_MAX), 0);

    EXPECT_EQ(bit_find_first_set_u32(0), 0);
    EXPECT_EQ(bit_find_first_set_u32(1), 1);
    EXPECT_EQ(bit_find_first_set_u32(2), 2);
    EXPECT_EQ(bit_find_first_set_u32(3), 1);
    EXPECT_EQ(bit_find_first_set_u32(64), 7);
    EXPECT_EQ(bit_find_first_set_u32(U32(1) << 31), 32);

    EXPECT_EQ(bit_find_first_set_u64(0), 0);
    EXPECT_EQ(bit_find_first_set_u64(1), 1);
    EXPECT_EQ(bit_find_first_set_u64(2), 2);
    EXPECT_EQ(bit_find_first_set_u64(3), 1);
    EXPECT_EQ(bit_find_first_set_u64(64), 7);
    EXPECT_EQ(bit_find_first_set_u64(U64(1) << 63), 64);
}

static void test_path_dirname_and_path_basename(void)
{
    static const struct {
        const char *path;
        const char *dirname;
        const char *basename;
    } tests[] = {
        {"/home/user/example.txt", "/home/user", "example.txt"},
        {"./../dir/example.txt", "./../dir", "example.txt"},
        {"/usr/bin/", "/usr/bin", ""},
        {"example.txt", ".", "example.txt"},
        {"/usr/lib", "/usr", "lib"},
        {"/usr", "/", "usr"},
        {"usr", ".", "usr"},
        {"/", "/", ""},
        {".", ".", "."},
        {"..", ".", ".."},
    };
    FOR_EACH_I(i, tests) {
        char *dir = path_dirname(tests[i].path);
        IEXPECT_STREQ(dir, tests[i].dirname);
        free(dir);
        IEXPECT_STREQ(path_basename(tests[i].path), tests[i].basename);
    }
}

static void test_path_absolute(void)
{
    char *path = path_absolute("./build/../build/test/test-symlink");
    EXPECT_STREQ(path_basename(path), "README.md");
    free(path);
}

static void test_regexp_match(void)
{
    static const char buf[] = "fn(a);\n";

    PointerArray a = PTR_ARRAY_INIT;
    bool matched = regexp_match("^[a-z]+\\(", buf, sizeof(buf) - 1, &a);
    EXPECT_TRUE(matched);
    EXPECT_EQ(a.count, 1);
    EXPECT_STREQ(a.ptrs[0], "fn(");
    ptr_array_free(&a);

    ptr_array_init(&a, 0);
    matched = regexp_match("^[a-z]+\\([0-9]", buf, sizeof(buf) - 1, &a);
    EXPECT_FALSE(matched);
    EXPECT_EQ(a.count, 0);
    ptr_array_free(&a);
}

void test_util(void)
{
    test_ascii();
    test_string();
    test_string_view();
    test_number_width();
    test_buf_parse_ulong();
    test_str_to_int();
    test_str_to_size();
    test_u_char_width();
    test_u_to_lower();
    test_u_is_upper();
    test_u_is_special_whitespace();
    test_u_is_unprintable();
    test_u_str_width();
    test_u_set_char();
    test_u_set_ctrl();
    test_u_prev_char();
    test_hashset();
    test_round_up();
    test_bitop();
    test_path_dirname_and_path_basename();
    test_path_absolute();
    test_regexp_match();
}
