#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include "test.h"
#include "../src/util/ascii.h"
#include "../src/util/bit.h"
#include "../src/util/checked-arith.h"
#include "../src/util/hashset.h"
#include "../src/util/path.h"
#include "../src/util/ptr-array.h"
#include "../src/util/readfile.h"
#include "../src/util/str-util.h"
#include "../src/util/string-view.h"
#include "../src/util/string.h"
#include "../src/util/strtonum.h"
#include "../src/util/unicode.h"
#include "../src/util/utf8.h"
#include "../src/util/xmalloc.h"
#include "../src/util/xsnprintf.h"

static void test_macros(void)
{
    EXPECT_EQ(STRLEN(""), 0);
    EXPECT_EQ(STRLEN("a"), 1);
    EXPECT_EQ(STRLEN("123456789"), 9);

    EXPECT_EQ(ARRAY_COUNT(""), 1);
    EXPECT_EQ(ARRAY_COUNT("a"), 2);
    EXPECT_EQ(ARRAY_COUNT("123456789"), 10);

    EXPECT_EQ(MIN(0, 1), 0);
    EXPECT_EQ(MIN(99, 100), 99);
    EXPECT_EQ(MIN(-10, 10), -10);

    EXPECT_TRUE(IS_POWER_OF_2(1));
    EXPECT_TRUE(IS_POWER_OF_2(2));
    EXPECT_TRUE(IS_POWER_OF_2(4));
    EXPECT_TRUE(IS_POWER_OF_2(8));
    EXPECT_TRUE(IS_POWER_OF_2(4096));
    EXPECT_TRUE(IS_POWER_OF_2(8192));
    EXPECT_TRUE(IS_POWER_OF_2(1ULL << 63));
    EXPECT_FALSE(IS_POWER_OF_2(0));
    EXPECT_FALSE(IS_POWER_OF_2(3));
    EXPECT_FALSE(IS_POWER_OF_2(12));
    EXPECT_FALSE(IS_POWER_OF_2(-10));
}

static void test_ascii(void)
{
    EXPECT_EQ(ascii_tolower('A'), 'a');
    EXPECT_EQ(ascii_tolower('F'), 'f');
    EXPECT_EQ(ascii_tolower('Z'), 'z');
    EXPECT_EQ(ascii_tolower('a'), 'a');
    EXPECT_EQ(ascii_tolower('f'), 'f');
    EXPECT_EQ(ascii_tolower('z'), 'z');
    EXPECT_EQ(ascii_tolower('9'), '9');
    EXPECT_EQ(ascii_tolower('~'), '~');
    EXPECT_EQ(ascii_tolower('\0'), '\0');

    EXPECT_EQ(ascii_toupper('a'), 'A');
    EXPECT_EQ(ascii_toupper('f'), 'F');
    EXPECT_EQ(ascii_toupper('z'), 'Z');
    EXPECT_EQ(ascii_toupper('A'), 'A');
    EXPECT_EQ(ascii_toupper('F'), 'F');
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

    EXPECT_TRUE(ascii_isdigit('0'));
    EXPECT_TRUE(ascii_isdigit('1'));
    EXPECT_TRUE(ascii_isdigit('9'));
    EXPECT_FALSE(ascii_isdigit('a'));
    EXPECT_FALSE(ascii_isdigit('f'));
    EXPECT_FALSE(ascii_isdigit('/'));
    EXPECT_FALSE(ascii_isdigit(':'));
    EXPECT_FALSE(ascii_isdigit('\0'));
    EXPECT_FALSE(ascii_isdigit(0xFF));

    EXPECT_TRUE(ascii_isxdigit('0'));
    EXPECT_TRUE(ascii_isxdigit('1'));
    EXPECT_TRUE(ascii_isxdigit('9'));
    EXPECT_TRUE(ascii_isxdigit('a'));
    EXPECT_TRUE(ascii_isxdigit('A'));
    EXPECT_TRUE(ascii_isxdigit('f'));
    EXPECT_TRUE(ascii_isxdigit('F'));
    EXPECT_FALSE(ascii_isxdigit('g'));
    EXPECT_FALSE(ascii_isxdigit('G'));
    EXPECT_FALSE(ascii_isxdigit('@'));
    EXPECT_FALSE(ascii_isxdigit('/'));
    EXPECT_FALSE(ascii_isxdigit(':'));
    EXPECT_FALSE(ascii_isxdigit('\0'));
    EXPECT_FALSE(ascii_isxdigit(0xFF));

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

    EXPECT_TRUE(is_regex_special_char('('));
    EXPECT_TRUE(is_regex_special_char(')'));
    EXPECT_TRUE(is_regex_special_char('*'));
    EXPECT_TRUE(is_regex_special_char('+'));
    EXPECT_TRUE(is_regex_special_char('.'));
    EXPECT_TRUE(is_regex_special_char('?'));
    EXPECT_TRUE(is_regex_special_char('['));
    EXPECT_TRUE(is_regex_special_char('{'));
    EXPECT_TRUE(is_regex_special_char('|'));
    EXPECT_TRUE(is_regex_special_char('\\'));
    EXPECT_FALSE(is_regex_special_char('"'));
    EXPECT_FALSE(is_regex_special_char('$'));
    EXPECT_FALSE(is_regex_special_char('&'));
    EXPECT_FALSE(is_regex_special_char(','));
    EXPECT_FALSE(is_regex_special_char('0'));
    EXPECT_FALSE(is_regex_special_char('@'));
    EXPECT_FALSE(is_regex_special_char('A'));
    EXPECT_FALSE(is_regex_special_char('\''));
    EXPECT_FALSE(is_regex_special_char(']'));
    EXPECT_FALSE(is_regex_special_char('^'));
    EXPECT_FALSE(is_regex_special_char('_'));
    EXPECT_FALSE(is_regex_special_char('z'));
    EXPECT_FALSE(is_regex_special_char('}'));
    EXPECT_FALSE(is_regex_special_char('~'));
    EXPECT_FALSE(is_regex_special_char(0x00));
    EXPECT_FALSE(is_regex_special_char(0x80));
    EXPECT_FALSE(is_regex_special_char(0xFF));

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

    EXPECT_TRUE(ascii_streq_icase("", ""));
    EXPECT_TRUE(ascii_streq_icase("a", "a"));
    EXPECT_TRUE(ascii_streq_icase("a", "A"));
    EXPECT_TRUE(ascii_streq_icase("z", "Z"));
    EXPECT_TRUE(ascii_streq_icase("cx", "CX"));
    EXPECT_TRUE(ascii_streq_icase("ABC..XYZ", "abc..xyz"));
    EXPECT_TRUE(ascii_streq_icase("Ctrl", "CTRL"));
    EXPECT_FALSE(ascii_streq_icase("a", ""));
    EXPECT_FALSE(ascii_streq_icase("", "a"));
    EXPECT_FALSE(ascii_streq_icase("Ctrl+", "CTRL"));
    EXPECT_FALSE(ascii_streq_icase("Ctrl", "CTRL+"));
    EXPECT_FALSE(ascii_streq_icase("Ctrl", "Ctr"));
    EXPECT_FALSE(ascii_streq_icase("Ctrl", "CtrM"));

    const char s1[8] = "Ctrl+Up";
    const char s2[8] = "CTRL+U_";
    EXPECT_TRUE(mem_equal_icase(s1, s2, 0));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 1));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 2));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 3));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 4));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 5));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 6));
    EXPECT_FALSE(mem_equal_icase(s1, s2, 7));
    EXPECT_FALSE(mem_equal_icase(s1, s2, 8));

    char *saved_locale = xstrdup(setlocale(LC_CTYPE, NULL));
    setlocale(LC_CTYPE, "C");
    for (int i = -1; i < 256; i++) {
        EXPECT_EQ(!!ascii_isalpha(i), !!isalpha(i));
        EXPECT_EQ(!!ascii_isalnum(i), !!isalnum(i));
        EXPECT_EQ(!!ascii_islower(i), !!islower(i));
        EXPECT_EQ(!!ascii_isupper(i), !!isupper(i));
        EXPECT_EQ(!!ascii_iscntrl(i), !!iscntrl(i));
        EXPECT_EQ(!!ascii_isdigit(i), !!isdigit(i));
        EXPECT_EQ(!!ascii_isblank(i), !!isblank(i));
        EXPECT_EQ(!!ascii_isprint(i), !!isprint(i));
        EXPECT_EQ(!!ascii_isxdigit(i), !!isxdigit(i));
        if (i != '\v' && i != '\f') {
            EXPECT_EQ(!!ascii_isspace(i), !!isspace(i));
        }
    }
    setlocale(LC_CTYPE, saved_locale);
    free(saved_locale);
}

static void test_string(void)
{
    String s = STRING_INIT;
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    EXPECT_PTREQ(s.buffer, NULL);

    char *cstr = string_clone_cstring(&s);
    EXPECT_STREQ(cstr, "");
    free(cstr);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    EXPECT_PTREQ(s.buffer, NULL);

    string_insert_ch(&s, 0, 0x1F4AF);
    EXPECT_EQ(s.len, 4);
    EXPECT_STREQ(string_borrow_cstring(&s), "\xF0\x9F\x92\xAF");

    string_add_str(&s, "test");
    EXPECT_EQ(s.len, 8);
    EXPECT_STREQ(string_borrow_cstring(&s), "\xF0\x9F\x92\xAFtest");

    string_remove(&s, 0, 5);
    EXPECT_EQ(s.len, 3);
    EXPECT_STREQ(string_borrow_cstring(&s), "est");

    string_make_space(&s, 0, 1);
    EXPECT_EQ(s.len, 4);
    s.buffer[0] = 't';
    EXPECT_STREQ(string_borrow_cstring(&s), "test");

    string_clear(&s);
    EXPECT_EQ(s.len, 0);
    string_insert_ch(&s, 0, 0x0E01);
    EXPECT_EQ(s.len, 3);
    EXPECT_STREQ(string_borrow_cstring(&s), "\xE0\xB8\x81");

    string_clear(&s);
    string_sprintf(&s, "%d %s\n", 88, "test");
    EXPECT_EQ(s.len, 8);
    EXPECT_STREQ(string_borrow_cstring(&s), "88 test\n");

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
    EXPECT_EQ(buf_parse_ulong("0", 1, &val), 1);
    EXPECT_EQ(val, 0);
    EXPECT_EQ(buf_parse_ulong("9876", 4, &val), 4);
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

static void test_u_is_cntrl(void)
{
    EXPECT_TRUE(u_is_cntrl(0x00));
    EXPECT_TRUE(u_is_cntrl(0x09));
    EXPECT_TRUE(u_is_cntrl(0x0D));
    EXPECT_TRUE(u_is_cntrl(0x1F));
    EXPECT_TRUE(u_is_cntrl(0x7F));
    EXPECT_TRUE(u_is_cntrl(0x80));
    EXPECT_TRUE(u_is_cntrl(0x81));
    EXPECT_TRUE(u_is_cntrl(0x9E));
    EXPECT_TRUE(u_is_cntrl(0x9F));
    EXPECT_FALSE(u_is_cntrl(0x20));
    EXPECT_FALSE(u_is_cntrl(0x21));
    EXPECT_FALSE(u_is_cntrl(0x7E));
    EXPECT_FALSE(u_is_cntrl(0xA0));
    EXPECT_FALSE(u_is_cntrl(0x41));
    EXPECT_FALSE(u_is_cntrl(0x61));
    EXPECT_FALSE(u_is_cntrl(0xFF));
}

static void test_u_is_zero_width(void)
{
    // Default ignorable codepoints:
    EXPECT_TRUE(u_is_zero_width(0x034F));
    EXPECT_TRUE(u_is_zero_width(0x061C));
    EXPECT_TRUE(u_is_zero_width(0x115F));
    EXPECT_TRUE(u_is_zero_width(0x1160));
    EXPECT_TRUE(u_is_zero_width(0x180B));
    EXPECT_TRUE(u_is_zero_width(0x200B));
    EXPECT_TRUE(u_is_zero_width(0x202E));
    EXPECT_TRUE(u_is_zero_width(0xFEFF));
    EXPECT_TRUE(u_is_zero_width(0xE0000));
    EXPECT_TRUE(u_is_zero_width(0xE0FFF));
    // Non-spacing marks:
    EXPECT_TRUE(u_is_zero_width(0x0300));
    EXPECT_TRUE(u_is_zero_width(0x0730));
    EXPECT_TRUE(u_is_zero_width(0x11839));
    EXPECT_TRUE(u_is_zero_width(0x1183A));
    EXPECT_TRUE(u_is_zero_width(0xE01EF));
    // Not zero-width:
    EXPECT_FALSE(u_is_zero_width(0x0000));
    EXPECT_FALSE(u_is_zero_width('Z'));
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
    MEMZERO(&buf);

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
    MEMZERO(&buf);

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

static void test_ptr_array(void)
{
    PointerArray a = PTR_ARRAY_INIT;
    ptr_array_add(&a, NULL);
    ptr_array_add(&a, NULL);
    ptr_array_add(&a, xstrdup("foo"));
    ptr_array_add(&a, NULL);
    ptr_array_add(&a, xstrdup("bar"));
    ptr_array_add(&a, NULL);
    ptr_array_add(&a, NULL);
    EXPECT_EQ(a.count, 7);

    ptr_array_trim_nulls(&a);
    EXPECT_EQ(a.count, 4);
    EXPECT_STREQ(a.ptrs[0], "foo");
    EXPECT_NULL(a.ptrs[1]);
    EXPECT_STREQ(a.ptrs[2], "bar");
    EXPECT_NULL(a.ptrs[3]);
    ptr_array_trim_nulls(&a);
    EXPECT_EQ(a.count, 4);

    ptr_array_free(&a);
    EXPECT_EQ(a.count, 0);
    ptr_array_trim_nulls(&a);
    EXPECT_EQ(a.count, 0);
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
    hashset_init(&set, ARRAY_COUNT(strings), false);
    EXPECT_NULL(hashset_get(&set, "foo", 3));

    hashset_add_many(&set, (char**)strings, ARRAY_COUNT(strings));
    EXPECT_NONNULL(hashset_get(&set, "\t\xff\x80\b", 4));
    EXPECT_NONNULL(hashset_get(&set, "foo", 3));
    EXPECT_NONNULL(hashset_get(&set, "Foo", 3));

    EXPECT_NULL(hashset_get(&set, "FOO", 3));
    EXPECT_NULL(hashset_get(&set, "", 0));
    EXPECT_NULL(hashset_get(&set, NULL, 0));
    EXPECT_NULL(hashset_get(&set, "\0", 1));

    const char *last_string = strings[ARRAY_COUNT(strings) - 1];
    EXPECT_NONNULL(hashset_get(&set, last_string, strlen(last_string)));

    FOR_EACH_I(i, strings) {
        const char *str = strings[i];
        const size_t len = strlen(str);
        EXPECT_NONNULL(hashset_get(&set, str, len));
        EXPECT_NULL(hashset_get(&set, str, len - 1));
        EXPECT_NULL(hashset_get(&set, str + 1, len - 1));
    }

    hashset_free(&set);
    MEMZERO(&set);

    hashset_init(&set, ARRAY_COUNT(strings), true);
    hashset_add_many(&set, (char**)strings, ARRAY_COUNT(strings));
    EXPECT_NONNULL(hashset_get(&set, "foo", 3));
    EXPECT_NONNULL(hashset_get(&set, "FOO", 3));
    EXPECT_NONNULL(hashset_get(&set, "fOO", 3));
    hashset_free(&set);
    MEMZERO(&set);

    // Check that hashset_add() returns existing entries instead of
    // inserting duplicates
    hashset_init(&set, 0, false);
    EXPECT_EQ(set.nr_entries, 0);
    HashSetEntry *e1 = hashset_add(&set, "foo", 3);
    EXPECT_EQ(e1->str_len, 3);
    EXPECT_STREQ(e1->str, "foo");
    EXPECT_EQ(set.nr_entries, 1);
    HashSetEntry *e2 = hashset_add(&set, "foo", 3);
    EXPECT_PTREQ(e1, e2);
    EXPECT_EQ(set.nr_entries, 1);
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

static void test_round_size_to_next_power_of_2(void)
{
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(0), 0);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(1), 1);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(2), 2);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(3), 4);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(4), 4);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(5), 8);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(8), 8);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(9), 16);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(17), 32);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(61), 64);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(64), 64);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(200), 256);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(1000), 1024);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(5500), 8192);
}

static void test_bitop(void)
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
        {"", ".", ""},
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
    char *path = path_absolute("///dev///");
    ASSERT_NONNULL(path);
    EXPECT_STREQ(path, "/dev/");
    free(path);

    const char *linkpath = "./build/../build/test/test-symlink";
    if (symlink("../../README.md", linkpath) != 0) {
        TEST_FAIL("symlink() failed: %s", strerror(errno));
        return;
    }

    path = path_absolute(linkpath);
    EXPECT_EQ(unlink(linkpath), 0);
    ASSERT_NONNULL(path);
    EXPECT_STREQ(path_basename(path), "README.md");
    free(path);

    char buf[8192 + 1];
    memset(buf, 'a', sizeof(buf));
    buf[0] = '/';
    buf[8192] = '\0';
    errno = 0;
    EXPECT_NULL(path_absolute(buf));
    EXPECT_EQ(errno, ENAMETOOLONG);
}

static void test_path_parent(void)
{
    StringView sv = STRING_VIEW("/a/foo/bar/etc/file");
    EXPECT_EQ(sv.length, 19);
    EXPECT_TRUE(path_parent(&sv));
    EXPECT_EQ(sv.length, 14);
    EXPECT_TRUE(path_parent(&sv));
    EXPECT_EQ(sv.length, 10);
    EXPECT_TRUE(path_parent(&sv));
    EXPECT_EQ(sv.length, 6);
    EXPECT_TRUE(path_parent(&sv));
    EXPECT_EQ(sv.length, 2);
    EXPECT_TRUE(path_parent(&sv));
    EXPECT_EQ(sv.length, 1);
    EXPECT_FALSE(path_parent(&sv));
    EXPECT_EQ(sv.length, 1);

    StringView sv2 = STRING_VIEW("/etc/foo/x/y/");
    EXPECT_EQ(sv2.length, 13);
    EXPECT_TRUE(path_parent(&sv2));
    EXPECT_EQ(sv2.length, 10);
    EXPECT_TRUE(path_parent(&sv2));
    EXPECT_EQ(sv2.length, 8);
    EXPECT_TRUE(path_parent(&sv2));
    EXPECT_EQ(sv2.length, 4);
    EXPECT_TRUE(path_parent(&sv2));
    EXPECT_EQ(sv2.length, 1);
    EXPECT_FALSE(path_parent(&sv2));
    EXPECT_EQ(sv2.length, 1);
}

static void test_size_multiply_overflows(void)
{
    size_t r = 0;
    EXPECT_FALSE(size_multiply_overflows(10, 20, &r));
    EXPECT_UINT_EQ(r, 200);
    EXPECT_FALSE(size_multiply_overflows(0, 0, &r));
    EXPECT_UINT_EQ(r, 0);
    EXPECT_FALSE(size_multiply_overflows(1, 0, &r));
    EXPECT_UINT_EQ(r, 0);
    EXPECT_FALSE(size_multiply_overflows(0, 1, &r));
    EXPECT_UINT_EQ(r, 0);
    EXPECT_FALSE(size_multiply_overflows(0, SIZE_MAX, &r));
    EXPECT_UINT_EQ(r, 0);
    EXPECT_FALSE(size_multiply_overflows(SIZE_MAX, 0, &r));
    EXPECT_UINT_EQ(r, 0);
    EXPECT_FALSE(size_multiply_overflows(1, SIZE_MAX, &r));
    EXPECT_UINT_EQ(r, SIZE_MAX);
    EXPECT_FALSE(size_multiply_overflows(2, SIZE_MAX / 3, &r));
    EXPECT_UINT_EQ(r, 2 * (SIZE_MAX / 3));
    EXPECT_TRUE(size_multiply_overflows(SIZE_MAX, 2, &r));
    EXPECT_TRUE(size_multiply_overflows(2, SIZE_MAX, &r));
    EXPECT_TRUE(size_multiply_overflows(3, SIZE_MAX / 2, &r));
    EXPECT_TRUE(size_multiply_overflows(32767, SIZE_MAX, &r));
    EXPECT_TRUE(size_multiply_overflows(SIZE_MAX, SIZE_MAX, &r));
    EXPECT_TRUE(size_multiply_overflows(SIZE_MAX, SIZE_MAX / 2, &r));
}

static void test_size_add_overflows(void)
{
    size_t r = 0;
    EXPECT_FALSE(size_add_overflows(10, 20, &r));
    EXPECT_UINT_EQ(r, 30);
    EXPECT_FALSE(size_add_overflows(SIZE_MAX, 0, &r));
    EXPECT_UINT_EQ(r, SIZE_MAX);
    EXPECT_TRUE(size_add_overflows(SIZE_MAX, 1, &r));
    EXPECT_TRUE(size_add_overflows(SIZE_MAX, 16, &r));
    EXPECT_TRUE(size_add_overflows(SIZE_MAX, SIZE_MAX, &r));
    EXPECT_TRUE(size_add_overflows(SIZE_MAX, SIZE_MAX / 2, &r));
}

static void test_mem_intern(void)
{
    const char *ptrs[256];
    char str[8];
    for (size_t i = 0; i < ARRAY_COUNT(ptrs); i++) {
        size_t len = xsnprintf(str, sizeof str, "%zu", i);
        ptrs[i] = mem_intern(str, len);
    }

    EXPECT_STREQ(ptrs[0], "0");
    EXPECT_STREQ(ptrs[1], "1");
    EXPECT_STREQ(ptrs[101], "101");
    EXPECT_STREQ(ptrs[255], "255");

    for (size_t i = 0; i < ARRAY_COUNT(ptrs); i++) {
        size_t len = xsnprintf(str, sizeof str, "%zu", i);
        const char *ptr = mem_intern(str, len);
        EXPECT_PTREQ(ptr, ptrs[i]);
    }
}

static void test_read_file(void)
{
    char *buf = NULL;
    ssize_t size = read_file("/dev", &buf);
    EXPECT_EQ(size, -1);
    EXPECT_EQ(errno, EISDIR);
    EXPECT_NULL(buf);

    size = read_file("test/data/3lines.txt", &buf);
    ASSERT_NONNULL(buf);
    EXPECT_EQ(size, 26);

    size_t pos = 0;
    const char *line = buf_next_line(buf, &pos, size);
    EXPECT_STREQ(line, "line #1");
    EXPECT_EQ(pos, 8);
    line = buf_next_line(buf, &pos, size);
    EXPECT_STREQ(line, " line #2");
    EXPECT_EQ(pos, 17);
    line = buf_next_line(buf, &pos, size);
    EXPECT_STREQ(line, "  line #3");
    EXPECT_EQ(pos, 26);

    free(buf);
}

DISABLE_WARNING("-Wmissing-prototypes")

void test_util(void)
{
    test_macros();
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
    test_u_is_cntrl();
    test_u_is_zero_width();
    test_u_is_special_whitespace();
    test_u_is_unprintable();
    test_u_str_width();
    test_u_set_char();
    test_u_set_ctrl();
    test_u_prev_char();
    test_ptr_array();
    test_hashset();
    test_round_up();
    test_round_size_to_next_power_of_2();
    test_bitop();
    test_path_dirname_and_path_basename();
    test_path_absolute();
    test_path_parent();
    test_size_multiply_overflows();
    test_size_add_overflows();
    test_mem_intern();
    test_read_file();
}
