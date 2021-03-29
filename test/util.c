#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"
#include "util/ascii.h"
#include "util/base64.h"
#include "util/checked-arith.h"
#include "util/hash.h"
#include "util/hashmap.h"
#include "util/hashset.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/ptr-array.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/string.h"
#include "util/strtonum.h"
#include "util/unicode.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "util/xstdio.h"

static void test_util_macros(void)
{
    EXPECT_EQ(STRLEN(""), 0);
    EXPECT_EQ(STRLEN("a"), 1);
    EXPECT_EQ(STRLEN("123456789"), 9);

    EXPECT_EQ(ARRAY_COUNT(""), 1);
    EXPECT_EQ(ARRAY_COUNT("a"), 2);
    EXPECT_EQ(ARRAY_COUNT("123456789"), 10);

    const UNUSED char a2[] = {1, 2};
    const UNUSED int a3[] = {1, 2, 3};
    const UNUSED long long a4[] = {1, 2, 3, 4};
    EXPECT_EQ(ARRAY_COUNT(a2), 2);
    EXPECT_EQ(ARRAY_COUNT(a3), 3);
    EXPECT_EQ(ARRAY_COUNT(a4), 4);

    EXPECT_EQ(MIN(0, 1), 0);
    EXPECT_EQ(MIN(99, 100), 99);
    EXPECT_EQ(MIN(-10, 10), -10);

    EXPECT_EQ(MAX(0, 1), 1);
    EXPECT_EQ(MAX(99, 100), 100);
    EXPECT_EQ(MAX(-10, 10), 10);

    int n = snprintf(NULL, 0, "%d", INT_MIN);
    EXPECT_TRUE(n >= STRLEN("-2147483647"));
    EXPECT_TRUE(DECIMAL_STR_MAX(int) > n);
    n = snprintf(NULL, 0, "%llu", ULLONG_MAX);
    EXPECT_TRUE(n >= STRLEN("18446744073709551615"));
    EXPECT_TRUE(DECIMAL_STR_MAX(unsigned long long) > n);
}

static void test_IS_POWER_OF_2(void)
{
    EXPECT_TRUE(IS_POWER_OF_2(1));
    EXPECT_TRUE(IS_POWER_OF_2(2));
    EXPECT_TRUE(IS_POWER_OF_2(4));
    EXPECT_TRUE(IS_POWER_OF_2(8));
    EXPECT_TRUE(IS_POWER_OF_2(4096));
    EXPECT_TRUE(IS_POWER_OF_2(8192));
    EXPECT_TRUE(IS_POWER_OF_2(1ULL << 63));

    EXPECT_FALSE(IS_POWER_OF_2(0));
    EXPECT_FALSE(IS_POWER_OF_2(3));
    EXPECT_FALSE(IS_POWER_OF_2(5));
    EXPECT_FALSE(IS_POWER_OF_2(6));
    EXPECT_FALSE(IS_POWER_OF_2(7));
    EXPECT_FALSE(IS_POWER_OF_2(12));
    EXPECT_FALSE(IS_POWER_OF_2(15));
    EXPECT_FALSE(IS_POWER_OF_2(-2));
    EXPECT_FALSE(IS_POWER_OF_2(-10));

    const uintmax_t max_pow2 = UINTMAX_MAX & ~(UINTMAX_MAX >> 1);
    EXPECT_TRUE(max_pow2 >= 1ULL << 63);
    EXPECT_TRUE(IS_POWER_OF_2(max_pow2));
    EXPECT_UINT_EQ(max_pow2 << 1, 0);

    for (uintmax_t i = max_pow2; i > 4; i >>= 1) {
        EXPECT_TRUE(IS_POWER_OF_2(i));
        for (uintmax_t j = 1; j < 4; j++) {
            EXPECT_FALSE(IS_POWER_OF_2(i + j));
            EXPECT_FALSE(IS_POWER_OF_2(i - j));
        }
    }
}

static void test_xstreq(void)
{
    EXPECT_TRUE(xstreq("foo", "foo"));
    EXPECT_TRUE(xstreq("foo\0\n", "foo\0\0"));
    EXPECT_TRUE(xstreq("\0foo", "\0bar"));
    EXPECT_TRUE(xstreq(NULL, NULL));
    EXPECT_FALSE(xstreq("foo", "bar"));
    EXPECT_FALSE(xstreq("abc", "abcd"));
    EXPECT_FALSE(xstreq("abcd", "abc"));
    EXPECT_FALSE(xstreq(NULL, ""));
    EXPECT_FALSE(xstreq("", NULL));
}

static void test_hex_decode(void)
{
    EXPECT_EQ(hex_decode('0'), 0);
    EXPECT_EQ(hex_decode('1'), 1);
    EXPECT_EQ(hex_decode('9'), 9);
    EXPECT_EQ(hex_decode('a'), 10);
    EXPECT_EQ(hex_decode('A'), 10);
    EXPECT_EQ(hex_decode('f'), 15);
    EXPECT_EQ(hex_decode('F'), 15);
    EXPECT_EQ(hex_decode('g'), HEX_INVALID);
    EXPECT_EQ(hex_decode('G'), HEX_INVALID);
    EXPECT_EQ(hex_decode('@'), HEX_INVALID);
    EXPECT_EQ(hex_decode('/'), HEX_INVALID);
    EXPECT_EQ(hex_decode(':'), HEX_INVALID);
    EXPECT_EQ(hex_decode(' '), HEX_INVALID);
    EXPECT_EQ(hex_decode('~'), HEX_INVALID);
    EXPECT_EQ(hex_decode('\0'), HEX_INVALID);
    EXPECT_EQ(hex_decode(0xFF), HEX_INVALID);
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

    // Query the current locale
    const char *locale = setlocale(LC_CTYPE, NULL);
    ASSERT_NONNULL(locale);

    // Copy the locale string (which may be in static storage)
    char *saved_locale = xstrdup(locale);

    // Check that the ascii_is*() functions behave like their corresponding
    // <ctype.h> macros, when in the standard "C" locale
    ASSERT_NONNULL(setlocale(LC_CTYPE, "C"));
    for (int i = -1; i < 256; i++) {
        EXPECT_EQ(ascii_isalpha(i), !!isalpha(i));
        EXPECT_EQ(ascii_isalnum(i), !!isalnum(i));
        EXPECT_EQ(ascii_islower(i), !!islower(i));
        EXPECT_EQ(ascii_isupper(i), !!isupper(i));
        EXPECT_EQ(ascii_iscntrl(i), !!iscntrl(i));
        EXPECT_EQ(ascii_isdigit(i), !!isdigit(i));
        EXPECT_EQ(ascii_isblank(i), !!isblank(i));
        EXPECT_EQ(ascii_isprint(i), !!isprint(i));
        EXPECT_EQ(hex_decode(i) <= 0xF, !!isxdigit(i));
        EXPECT_EQ(is_alpha_or_underscore(i), !!isalpha(i) || i == '_');
        EXPECT_EQ(is_alnum_or_underscore(i), !!isalnum(i) || i == '_');
        if (i != '\v' && i != '\f') {
            EXPECT_EQ(ascii_isspace(i), !!isspace(i));
            EXPECT_EQ(ascii_is_nonspace_cntrl(i), !!iscntrl(i) && !isspace(i));
        }
        if (i != -1) {
            EXPECT_EQ(ascii_tolower(i), tolower(i));
            EXPECT_EQ(ascii_toupper(i), toupper(i));
        }
    }

    // Restore the original locale
    setlocale(LC_CTYPE, saved_locale);
    free(saved_locale);
}

static void test_base64(void)
{
    EXPECT_EQ(base64_decode('A'), 0);
    EXPECT_EQ(base64_decode('Z'), 25);
    EXPECT_EQ(base64_decode('a'), 26);
    EXPECT_EQ(base64_decode('z'), 51);
    EXPECT_EQ(base64_decode('0'), 52);
    EXPECT_EQ(base64_decode('9'), 61);
    EXPECT_EQ(base64_decode('+'), 62);
    EXPECT_EQ(base64_decode('/'), 63);
    EXPECT_EQ(base64_decode('='), BASE64_PADDING);
    EXPECT_EQ(base64_decode(0x00), BASE64_INVALID);
    EXPECT_EQ(base64_decode(0xFF), BASE64_INVALID);

    for (unsigned int i = 'A'; i <= 'Z'; i++) {
        IEXPECT_EQ(base64_decode(i), i - 'A');
    }

    for (unsigned int i = 'a'; i <= 'z'; i++) {
        IEXPECT_EQ(base64_decode(i), (i - 'a') + 26);
    }

    for (unsigned int i = '0'; i <= '9'; i++) {
        IEXPECT_EQ(base64_decode(i), (i - '0') + 52);
    }

    for (unsigned int i = 0; i < 256; i++) {
        unsigned int val = base64_decode(i);
        if (ascii_isalnum(i) || i == '+' || i == '/') {
            IEXPECT_EQ(val, val & 63);
        } else {
            IEXPECT_EQ(val, val & 192);
        }
    }
}

static void test_string(void)
{
    String s = STRING_INIT;
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    EXPECT_NULL(s.buffer);

    char *cstr = string_clone_cstring(&s);
    EXPECT_STREQ(cstr, "");
    free(cstr);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    EXPECT_NULL(s.buffer);

    string_insert_ch(&s, 0, 0x1F4AF);
    EXPECT_EQ(s.len, 4);
    EXPECT_STREQ(string_borrow_cstring(&s), "\xF0\x9F\x92\xAF");

    string_append_cstring(&s, "test");
    EXPECT_EQ(s.len, 8);
    EXPECT_STREQ(string_borrow_cstring(&s), "\xF0\x9F\x92\xAFtest");

    string_remove(&s, 0, 5);
    EXPECT_EQ(s.len, 3);
    EXPECT_STREQ(string_borrow_cstring(&s), "est");

    string_insert_ch(&s, 0, 't');
    EXPECT_EQ(s.len, 4);
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
    EXPECT_NULL(s.buffer);

    for (size_t i = 0; i < 40; i++) {
        string_append_byte(&s, 'a');
    }

    EXPECT_EQ(s.len, 40);
    cstr = string_steal_cstring(&s);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    EXPECT_NULL(s.buffer);
    EXPECT_STREQ(cstr, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    free(cstr);

    s = string_new(12);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 16);
    ASSERT_NONNULL(s.buffer);

    string_append_cstring(&s, "123");
    EXPECT_STREQ(string_borrow_cstring(&s), "123");
    EXPECT_EQ(s.len, 3);

    string_append_string(&s, &s);
    EXPECT_STREQ(string_borrow_cstring(&s), "123123");
    EXPECT_EQ(s.len, 6);

    string_insert_buf(&s, 2, STRN("foo"));
    EXPECT_STREQ(string_borrow_cstring(&s), "12foo3123");
    EXPECT_EQ(s.len, 9);

    cstr = string_clone_cstring(&s);
    EXPECT_STREQ(cstr, "12foo3123");

    EXPECT_EQ(string_insert_ch(&s, 0, '>'), 1);
    EXPECT_STREQ(string_borrow_cstring(&s), ">12foo3123");
    EXPECT_EQ(s.len, 10);

    string_free(&s);
    EXPECT_NULL(s.buffer);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    EXPECT_STREQ(cstr, "12foo3123");
    free(cstr);
}

static void test_string_view(void)
{
    const StringView sv1 = STRING_VIEW("testing");
    EXPECT_TRUE(strview_equal_cstring(&sv1, "testing"));
    EXPECT_FALSE(strview_equal_cstring(&sv1, "testin"));
    EXPECT_FALSE(strview_equal_cstring(&sv1, "TESTING"));
    EXPECT_TRUE(strview_has_prefix(&sv1, "test"));
    EXPECT_TRUE(strview_has_prefix_icase(&sv1, "TEst"));
    EXPECT_FALSE(strview_has_prefix(&sv1, "TEst"));
    EXPECT_FALSE(strview_has_prefix_icase(&sv1, "TEst_"));

    const StringView sv2 = string_view(sv1.data, sv1.length);
    EXPECT_TRUE(strview_equal(&sv1, &sv2));

    const StringView sv3 = STRING_VIEW("\0test\0 ...");
    EXPECT_TRUE(strview_equal_strn(&sv3, "\0test\0 ...", 10));

    const StringView sv4 = sv3;
    EXPECT_TRUE(strview_equal(&sv4, &sv3));

    const StringView sv5 = strview_from_cstring("foobar");
    EXPECT_TRUE(strview_equal_cstring(&sv5, "foobar"));
    EXPECT_TRUE(strview_has_prefix(&sv5, "foo"));
    EXPECT_FALSE(strview_equal_cstring(&sv5, "foo"));

    StringView sv6 = strview_from_cstring("\t  \t\t ");
    EXPECT_TRUE(strview_isblank(&sv6));
    sv6.length = 0;
    EXPECT_TRUE(strview_isblank(&sv6));
    sv6 = strview_from_cstring("    \t .  ");
    EXPECT_FALSE(strview_isblank(&sv6));
    sv6 = strview_from_cstring("\n");
    EXPECT_FALSE(strview_isblank(&sv6));
    sv6 = strview_from_cstring("  \r ");
    EXPECT_FALSE(strview_isblank(&sv6));
}

static void test_get_delim(void)
{
    static const char input[] = "-x-y-foo--bar--";
    static const char parts[][4] = {"", "x", "y", "foo", "", "bar", "", ""};
    const size_t nparts = ARRAY_COUNT(parts);
    const size_t part_size = ARRAY_COUNT(parts[0]);

    size_t idx = 0;
    for (size_t pos = 0, len = sizeof(input) - 1; pos < len; idx++) {
        const StringView sv = get_delim(input, &pos, len, '-');
        ASSERT_TRUE(idx < nparts);
        ASSERT_TRUE(parts[idx][part_size - 1] == '\0');
        EXPECT_TRUE(strview_equal_cstring(&sv, parts[idx]));
    }

    EXPECT_EQ(idx, nparts - 1);
}

static void test_size_str_width(void)
{
    EXPECT_EQ(size_str_width(0), 1);
    EXPECT_EQ(size_str_width(1), 1);
    EXPECT_EQ(size_str_width(9), 1);
    EXPECT_EQ(size_str_width(19), 2);
    EXPECT_EQ(size_str_width(425), 3);
    EXPECT_EQ(size_str_width(12345), 5);
    EXPECT_EQ(size_str_width(2147483647), 10);
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

static void test_umax_to_str(void)
{
    EXPECT_STREQ(umax_to_str(0), "0");
    EXPECT_STREQ(umax_to_str(1), "1");
    EXPECT_STREQ(umax_to_str(7), "7");
    EXPECT_STREQ(umax_to_str(99), "99");
    EXPECT_STREQ(umax_to_str(111), "111");
    EXPECT_STREQ(umax_to_str(1000), "1000");
    EXPECT_STREQ(umax_to_str(20998), "20998");

    uintmax_t x = UINTMAX_MAX;
    char ref[DECIMAL_STR_MAX(x)];
    xsnprintf(ref, sizeof ref, "%ju", x);
    EXPECT_STREQ(umax_to_str(x), ref);
    x--;
    xsnprintf(ref, sizeof ref, "%ju", x);
    EXPECT_STREQ(umax_to_str(x), ref);
}

static void test_uint_to_str(void)
{
    EXPECT_STREQ(uint_to_str(0), "0");
    EXPECT_STREQ(uint_to_str(1), "1");
    EXPECT_STREQ(uint_to_str(9), "9");
    EXPECT_STREQ(uint_to_str(10), "10");
    EXPECT_STREQ(uint_to_str(11), "11");
    EXPECT_STREQ(uint_to_str(99), "99");
    EXPECT_STREQ(uint_to_str(100), "100");
    EXPECT_STREQ(uint_to_str(101), "101");
    EXPECT_STREQ(uint_to_str(21904), "21904");

    // See test_posix_sanity()
    static_assert(sizeof(unsigned int) >= 4);
    static_assert(CHAR_BIT == 8);
    static_assert(UINT_MAX >= 4294967295u);
    static_assert(4294967295u == 0xFFFFFFFF);
    EXPECT_STREQ(uint_to_str(4294967295u), "4294967295");
}

static void test_buf_uint_to_str(void)
{
    char buf[DECIMAL_STR_MAX(unsigned int)];
    EXPECT_EQ(buf_uint_to_str(0, buf), 1);
    EXPECT_STREQ(buf, "0");
    EXPECT_EQ(buf_uint_to_str(1, buf), 1);
    EXPECT_STREQ(buf, "1");
    EXPECT_EQ(buf_uint_to_str(9, buf), 1);
    EXPECT_STREQ(buf, "9");
    EXPECT_EQ(buf_uint_to_str(129, buf), 3);
    EXPECT_STREQ(buf, "129");
    EXPECT_EQ(buf_uint_to_str(21904, buf), 5);
    EXPECT_STREQ(buf, "21904");
    static_assert(sizeof(buf) > 10);
    EXPECT_EQ(buf_uint_to_str(4294967295u, buf), 10);
    EXPECT_STREQ(buf, "4294967295");
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
    EXPECT_EQ(u_char_width(0x30000), 2);

    // Double width but unassigned (rendered as <xx> -- 4 columns)
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

static void test_u_to_upper(void)
{
    EXPECT_EQ(u_to_upper('a'), 'A');
    EXPECT_EQ(u_to_upper('z'), 'Z');
    EXPECT_EQ(u_to_upper('A'), 'A');
    EXPECT_EQ(u_to_upper('0'), '0');
    EXPECT_EQ(u_to_upper('~'), '~');
    EXPECT_EQ(u_to_upper('@'), '@');
    EXPECT_EQ(u_to_upper('\0'), '\0');
}

static void test_u_is_lower(void)
{
    EXPECT_TRUE(u_is_lower('a'));
    EXPECT_TRUE(u_is_lower('z'));
    EXPECT_FALSE(u_is_lower('A'));
    EXPECT_FALSE(u_is_lower('Z'));
    EXPECT_FALSE(u_is_lower('0'));
    EXPECT_FALSE(u_is_lower('9'));
    EXPECT_FALSE(u_is_lower('@'));
    EXPECT_FALSE(u_is_lower('['));
    EXPECT_FALSE(u_is_lower('{'));
    EXPECT_FALSE(u_is_lower('\0'));
    EXPECT_FALSE(u_is_lower('\t'));
    EXPECT_FALSE(u_is_lower(' '));
    EXPECT_FALSE(u_is_lower(0x1F315));
    EXPECT_FALSE(u_is_lower(0x10ffff));

    /*
    Even if SANE_WCTYPE is defined, we still can't make many assumptions
    about the iswlower(3) implementation, since it depends on all kinds
    of factors out of our control. Otherwise it'd be reasonable to test
    something like:

    EXPECT_TRUE(u_is_lower(0x00E0));
    EXPECT_TRUE(u_is_lower(0x00E7));
    */
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
    EXPECT_FALSE(u_is_upper(0x1F315));
    EXPECT_FALSE(u_is_upper(0x10ffff));
    EXPECT_FALSE(u_is_upper(0x00E0));
    EXPECT_FALSE(u_is_upper(0x00E7));
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
    EXPECT_TRUE(u_is_unprintable(0x10FFFD));

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

static void test_u_set_char_raw(void)
{
    unsigned char buf[16];
    size_t i;
    MEMZERO(&buf);

    i = 0;
    u_set_char_raw(buf, &i, 'a');
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], 'a');

    i = 0;
    u_set_char_raw(buf, &i, '\0');
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], '\0');

    i = 0;
    u_set_char_raw(buf, &i, 0x1F);
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], 0x1F);

    i = 0;
    u_set_char_raw(buf, &i, 0x7F);
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], 0x7F);

    i = 0;
    u_set_char_raw(buf, &i, 0x7FF);
    EXPECT_EQ(i, 2);
    EXPECT_EQ(buf[0], 0xDF);
    EXPECT_EQ(buf[1], 0xBF);

    i = 0;
    u_set_char_raw(buf, &i, 0xFF45);
    EXPECT_EQ(i, 3);
    EXPECT_EQ(buf[0], 0xEF);
    EXPECT_EQ(buf[1], 0xBD);
    EXPECT_EQ(buf[2], 0x85);

    i = 0;
    u_set_char_raw(buf, &i, 0x1F311);
    EXPECT_EQ(i, 4);
    EXPECT_EQ(buf[0], 0xF0);
    EXPECT_EQ(buf[1], 0x9F);
    EXPECT_EQ(buf[2], 0x8C);
    EXPECT_EQ(buf[3], 0x91);

    i = 0;
    buf[1] = 0x88;
    u_set_char_raw(buf, &i, 0x110000);
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], 0);
    EXPECT_EQ(buf[1], 0x88);

    i = 0;
    u_set_char_raw(buf, &i, 0x110042);
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], 0x42);
    EXPECT_EQ(buf[1], 0x88);
}

static void test_u_set_char(void)
{
    unsigned char buf[16];
    size_t i;
    MEMZERO(&buf);

    i = 0;
    u_set_char(buf, &i, 'a');
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], 'a');

    i = 0;
    u_set_char(buf, &i, 0x00DF);
    EXPECT_EQ(i, 2);
    EXPECT_EQ(buf[0], 0xC3);
    EXPECT_EQ(buf[1], 0x9F);

    i = 0;
    u_set_char(buf, &i, 0x0E01);
    EXPECT_EQ(i, 3);
    EXPECT_EQ(buf[0], 0xE0);
    EXPECT_EQ(buf[1], 0xB8);
    EXPECT_EQ(buf[2], 0x81);

    i = 0;
    u_set_char(buf, &i, 0x1F914);
    EXPECT_EQ(i, 4);
    EXPECT_EQ(buf[0], 0xF0);
    EXPECT_EQ(buf[1], 0x9F);
    EXPECT_EQ(buf[2], 0xA4);
    EXPECT_EQ(buf[3], 0x94);

    i = 0;
    u_set_char(buf, &i, 0x10FFFF);
    EXPECT_EQ(i, 4);
    EXPECT_EQ(buf[0], '<');
    EXPECT_EQ(buf[1], '?');
    EXPECT_EQ(buf[2], '?');
    EXPECT_EQ(buf[3], '>');

    i = 0;
    u_set_char(buf, &i, '\0');
    EXPECT_EQ(i, 2);
    EXPECT_EQ(buf[0], '^');
    EXPECT_EQ(buf[1], '@');

    i = 0;
    u_set_char(buf, &i, '\t');
    EXPECT_EQ(i, 2);
    EXPECT_EQ(buf[0], '^');
    EXPECT_EQ(buf[1], 'I');

    i = 0;
    u_set_char(buf, &i, 0x1F);
    EXPECT_EQ(i, 2);
    EXPECT_EQ(buf[0], '^');
    EXPECT_EQ(buf[1], '_');

    i = 0;
    u_set_char(buf, &i, 0x7F);
    EXPECT_EQ(i, 2);
    EXPECT_EQ(buf[0], '^');
    EXPECT_EQ(buf[1], '?');

    i = 0;
    u_set_char(buf, &i, 0x80);
    EXPECT_EQ(i, 4);
    EXPECT_EQ(buf[0], '<');
    EXPECT_EQ(buf[1], '?');
    EXPECT_EQ(buf[2], '?');
    EXPECT_EQ(buf[3], '>');

    i = 0;
    u_set_char(buf, &i, 0x7E);
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], '~');

    i = 0;
    u_set_char(buf, &i, 0x20);
    EXPECT_EQ(i, 1);
    EXPECT_EQ(buf[0], ' ');
}

static void test_u_prev_char(void)
{
    const unsigned char *buf = "\xE6\xB7\xB1\xE5\x9C\xB3\xE5\xB8\x82"; // 深圳市
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

    buf = "\xF0\x9F\xA5\xA3\xF0\x9F\xA5\xA4"; // 🥣🥤
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
    ptr_array_append(&a, NULL);
    ptr_array_append(&a, NULL);
    ptr_array_append(&a, xstrdup("foo"));
    ptr_array_append(&a, NULL);
    ptr_array_append(&a, xstrdup("bar"));
    ptr_array_append(&a, NULL);
    ptr_array_append(&a, NULL);
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

static void test_ptr_array_move(void)
{
    PointerArray a = PTR_ARRAY_INIT;
    ptr_array_append(&a, xstrdup("A"));
    ptr_array_append(&a, xstrdup("B"));
    ptr_array_append(&a, xstrdup("C"));
    ptr_array_append(&a, xstrdup("D"));
    ptr_array_append(&a, xstrdup("E"));
    ptr_array_append(&a, xstrdup("F"));
    EXPECT_EQ(a.count, 6);

    ptr_array_move(&a, 0, 1);
    EXPECT_STREQ(a.ptrs[0], "B");
    EXPECT_STREQ(a.ptrs[1], "A");

    ptr_array_move(&a, 1, 0);
    EXPECT_STREQ(a.ptrs[0], "A");
    EXPECT_STREQ(a.ptrs[1], "B");

    ptr_array_move(&a, 1, 5);
    EXPECT_STREQ(a.ptrs[0], "A");
    EXPECT_STREQ(a.ptrs[1], "C");
    EXPECT_STREQ(a.ptrs[2], "D");
    EXPECT_STREQ(a.ptrs[3], "E");
    EXPECT_STREQ(a.ptrs[4], "F");
    EXPECT_STREQ(a.ptrs[5], "B");

    ptr_array_move(&a, 5, 1);
    EXPECT_STREQ(a.ptrs[0], "A");
    EXPECT_STREQ(a.ptrs[1], "B");
    EXPECT_STREQ(a.ptrs[2], "C");
    EXPECT_STREQ(a.ptrs[3], "D");
    EXPECT_STREQ(a.ptrs[4], "E");
    EXPECT_STREQ(a.ptrs[5], "F");

    ptr_array_move(&a, 0, 5);
    EXPECT_STREQ(a.ptrs[0], "B");
    EXPECT_STREQ(a.ptrs[1], "C");
    EXPECT_STREQ(a.ptrs[2], "D");
    EXPECT_STREQ(a.ptrs[3], "E");
    EXPECT_STREQ(a.ptrs[4], "F");
    EXPECT_STREQ(a.ptrs[5], "A");

    ptr_array_move(&a, 5, 0);
    EXPECT_STREQ(a.ptrs[0], "A");
    EXPECT_STREQ(a.ptrs[1], "B");
    EXPECT_STREQ(a.ptrs[2], "C");
    EXPECT_STREQ(a.ptrs[3], "D");
    EXPECT_STREQ(a.ptrs[4], "E");
    EXPECT_STREQ(a.ptrs[5], "F");

    ptr_array_move(&a, 5, 0);
    EXPECT_STREQ(a.ptrs[0], "F");
    EXPECT_STREQ(a.ptrs[1], "A");
    EXPECT_STREQ(a.ptrs[2], "B");
    EXPECT_STREQ(a.ptrs[3], "C");
    EXPECT_STREQ(a.ptrs[4], "D");
    EXPECT_STREQ(a.ptrs[5], "E");

    ptr_array_move(&a, 4, 5);
    EXPECT_STREQ(a.ptrs[4], "E");
    EXPECT_STREQ(a.ptrs[5], "D");

    ptr_array_move(&a, 1, 3);
    EXPECT_STREQ(a.ptrs[1], "B");
    EXPECT_STREQ(a.ptrs[2], "C");
    EXPECT_STREQ(a.ptrs[3], "A");

    ptr_array_free(&a);
}

static void test_hashmap(void)
{
    static const char strings[][8] = {
        "foo", "bar", "quux", "etc", "",
        "A", "B", "C", "D", "E", "F", "G",
        "a", "b", "c", "d", "e", "f", "g",
        "test", "1234567", "..", "...",
        "\x01\x02\x03 \t\xfe\xff",
    };

    HashMap map;
    hashmap_init(&map, ARRAY_COUNT(strings));
    ASSERT_NONNULL(map.entries);
    EXPECT_EQ(map.mask, 31);
    EXPECT_EQ(map.count, 0);
    EXPECT_NULL(hashmap_find(&map, "foo"));

    static const char value[] = "VALUE";
    FOR_EACH_I(i, strings) {
        const char *key = strings[i];
        ASSERT_EQ(key[sizeof(strings[0]) - 1], '\0');
        hashmap_insert(&map, xstrdup(key), (void*)value);
        HashMapEntry *e = hashmap_find(&map, key);
        ASSERT_NONNULL(e);
        EXPECT_STREQ(e->key, key);
        EXPECT_PTREQ(e->value, value);
    }

    EXPECT_EQ(map.count, 24);
    EXPECT_EQ(map.mask, 31);

    HashMapIter it = hashmap_iter(&map);
    EXPECT_PTREQ(it.map, &map);
    EXPECT_NULL(it.entry);
    EXPECT_EQ(it.idx, 0);

    while (hashmap_next(&it)) {
        ASSERT_NONNULL(it.entry);
        ASSERT_NONNULL(it.entry->key);
        EXPECT_PTREQ(it.entry->value, value);
    }

    FOR_EACH_I(i, strings) {
        const char *key = strings[i];
        HashMapEntry *e = hashmap_find(&map, key);
        ASSERT_NONNULL(e);
        EXPECT_STREQ(e->key, key);
        EXPECT_PTREQ(e->value, value);

        EXPECT_PTREQ(hashmap_remove(&map, key), value);
        EXPECT_STREQ(e->key, "TOMBSTONE");
        EXPECT_NULL(hashmap_find(&map, key));
    }

    EXPECT_EQ(map.count, 0);
    it = hashmap_iter(&map);
    EXPECT_FALSE(hashmap_next(&it));

    hashmap_insert(&map, xstrdup("new"), (void*)value);
    ASSERT_NONNULL(hashmap_find(&map, "new"));
    EXPECT_STREQ(hashmap_find(&map, "new")->key, "new");
    EXPECT_EQ(map.count, 1);

    FOR_EACH_I(i, strings) {
        const char *key = strings[i];
        hashmap_insert(&map, xstrdup(key), (void*)value);
        HashMapEntry *e = hashmap_find(&map, key);
        ASSERT_NONNULL(e);
        EXPECT_STREQ(e->key, key);
        EXPECT_PTREQ(e->value, value);
    }

    EXPECT_EQ(map.count, 25);

    FOR_EACH_I(i, strings) {
        const char *key = strings[i];
        HashMapEntry *e = hashmap_find(&map, key);
        ASSERT_NONNULL(e);
        EXPECT_STREQ(e->key, key);
        EXPECT_PTREQ(hashmap_remove(&map, key), value);
        EXPECT_STREQ(e->key, "TOMBSTONE");
        EXPECT_NULL(hashmap_find(&map, key));
    }

    EXPECT_EQ(map.count, 1);
    EXPECT_NULL(hashmap_remove(&map, "non-existent-key"));
    EXPECT_EQ(map.count, 1);
    EXPECT_PTREQ(hashmap_remove(&map, "new"), value);
    EXPECT_EQ(map.count, 0);

    it = hashmap_iter(&map);
    EXPECT_FALSE(hashmap_next(&it));

    hashmap_free(&map, NULL);
    EXPECT_NULL(map.entries);
    EXPECT_EQ(map.count, 0);
    EXPECT_EQ(map.mask, 0);

    hashmap_init(&map, 0);
    ASSERT_NONNULL(map.entries);
    EXPECT_EQ(map.mask, 7);
    EXPECT_EQ(map.count, 0);
    hashmap_free(&map, NULL);
    EXPECT_NULL(map.entries);

    hashmap_init(&map, 13);
    ASSERT_NONNULL(map.entries);
    EXPECT_EQ(map.mask, 31);
    EXPECT_EQ(map.count, 0);

    for (size_t i = 1; i <= 380; i++) {
        char key[4];
        EXPECT_EQ(xsnprintf(key, sizeof key, "%zu", i), size_str_width(i));
        hashmap_insert(&map, xstrdup(key), (void*)value);
        HashMapEntry *e = hashmap_find(&map, key);
        ASSERT_NONNULL(e);
        EXPECT_STREQ(e->key, key);
        EXPECT_PTREQ(e->value, value);
    }

    EXPECT_EQ(map.count, 380);
    EXPECT_EQ(map.mask, 511);
    hashmap_free(&map, NULL);

    const char *val = "VAL";
    char *key = xstrdup("KEY");
    EXPECT_NULL(hashmap_insert_or_replace(&map, key, (char*)val));
    EXPECT_EQ(map.count, 1);
    EXPECT_STREQ(hashmap_get(&map, "KEY"), val);

    const char *new_val = "NEW";
    char *duplicate_key = xstrdup(key);
    EXPECT_PTREQ(val, hashmap_insert_or_replace(&map, duplicate_key, (char*)new_val));
    EXPECT_EQ(map.count, 1);
    EXPECT_STREQ(hashmap_get(&map, "KEY"), new_val);
    hashmap_free(&map, NULL);
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
    EXPECT_EQ(set.nr_entries, 0);
    EXPECT_NULL(hashset_get(&set, "foo", 3));
    FOR_EACH_I(i, strings) {
        hashset_add(&set, strings[i], strlen(strings[i]));
    }

    EXPECT_EQ(set.nr_entries, ARRAY_COUNT(strings));
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

    hashset_init(&set, 0, true);
    EXPECT_EQ(set.nr_entries, 0);
    hashset_add(&set, STRN("foo"));
    hashset_add(&set, STRN("Foo"));
    EXPECT_EQ(set.nr_entries, 1);
    EXPECT_NONNULL(hashset_get(&set, STRN("foo")));
    EXPECT_NONNULL(hashset_get(&set, STRN("FOO")));
    EXPECT_NONNULL(hashset_get(&set, STRN("fOO")));
    hashset_free(&set);
    MEMZERO(&set);

    // Check that hashset_add() returns existing entries instead of
    // inserting duplicates
    hashset_init(&set, 0, false);
    EXPECT_EQ(set.nr_entries, 0);
    HashSetEntry *e1 = hashset_add(&set, STRN("foo"));
    EXPECT_EQ(e1->str_len, 3);
    EXPECT_STREQ(e1->str, "foo");
    EXPECT_EQ(set.nr_entries, 1);
    HashSetEntry *e2 = hashset_add(&set, STRN("foo"));
    EXPECT_PTREQ(e1, e2);
    EXPECT_EQ(set.nr_entries, 1);
    hashset_free(&set);

    hashset_init(&set, 0, false);
    // Initial table size should be 16 (minimum + load factor + rounding)
    EXPECT_EQ(set.table_size, 16);
    for (size_t i = 1; i <= 80; i++) {
        char buf[4];
        size_t n = xsnprintf(buf, sizeof buf, "%zu", i);
        hashset_add(&set, buf, n);
    }
    EXPECT_EQ(set.nr_entries, 80);
    EXPECT_NONNULL(hashset_get(&set, STRN("1")));
    EXPECT_NONNULL(hashset_get(&set, STRN("80")));
    EXPECT_NULL(hashset_get(&set, STRN("0")));
    EXPECT_NULL(hashset_get(&set, STRN("81")));
    // Table size should be the first power of 2 larger than the number
    // of entries (including load factor adjustment)
    EXPECT_EQ(set.table_size, 128);
    hashset_free(&set);
}

static void test_round_size_to_next_multiple(void)
{
    EXPECT_EQ(round_size_to_next_multiple(3, 8), 8);
    EXPECT_EQ(round_size_to_next_multiple(8, 8), 8);
    EXPECT_EQ(round_size_to_next_multiple(9, 8), 16);
    EXPECT_EQ(round_size_to_next_multiple(0, 8), 0);
    EXPECT_EQ(round_size_to_next_multiple(0, 16), 0);
    EXPECT_EQ(round_size_to_next_multiple(1, 16), 16);
    EXPECT_EQ(round_size_to_next_multiple(123, 16), 128);
    EXPECT_EQ(round_size_to_next_multiple(4, 64), 64);
    EXPECT_EQ(round_size_to_next_multiple(80, 64), 128);
    EXPECT_EQ(round_size_to_next_multiple(256, 256), 256);
    EXPECT_EQ(round_size_to_next_multiple(257, 256), 512);
    EXPECT_EQ(round_size_to_next_multiple(8000, 256), 8192);
}

static void test_round_size_to_next_power_of_2(void)
{
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(0), 1);
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
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(65), 128);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(200), 256);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(1000), 1024);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(5500), 8192);
    const size_t size_max = (size_t)-1;
    const size_t pow2_max = size_max & ~(size_max >> 1);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(size_max >> 1), pow2_max);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(pow2_max), pow2_max);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(pow2_max - 1), pow2_max);
    // Note: returns 0 on overflow
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(pow2_max + 1), 0);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(size_max), 0);
    EXPECT_UINT_EQ(round_size_to_next_power_of_2(size_max - 1), 0);
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

static void test_relative_filename(void)
{
    static const struct rel_test {
        const char *cwd;
        const char *path;
        const char *result;
    } tests[] = { // NOTE: at most 2 ".." components allowed in relative name
        { "/", "/", "/" },
        { "/", "/file", "file" },
        { "/a/b/c/d", "/a/b/file", "../../file" },
        { "/a/b/c/d/e", "/a/b/file", "/a/b/file" },
        { "/a/foobar", "/a/foo/file", "../foo/file" },
    };
    FOR_EACH_I(i, tests) {
        char *result = relative_filename(tests[i].path, tests[i].cwd);
        IEXPECT_STREQ(tests[i].result, result);
        free(result);
    }
}

static void test_path_absolute(void)
{
    char *path = path_absolute("///dev///");
    EXPECT_STREQ(path, "/dev");
    free(path);

    path = path_absolute("///dev///..///dev//null");
    EXPECT_STREQ(path, "/dev/null");
    free(path);

    path = path_absolute("///dev//n0nexist3nt-file");
    EXPECT_STREQ(path, "/dev/n0nexist3nt-file");
    free(path);

    path = path_absolute("///../..//./");
    EXPECT_STREQ(path, "/");
    free(path);

    path = path_absolute("/");
    EXPECT_STREQ(path, "/");
    free(path);

    path = path_absolute("");
    EXPECT_STREQ(path, NULL);
    free(path);

    const char *linkpath = "./build/../build/test/test-symlink";
    if (symlink("../../README.md", linkpath) != 0) {
        TEST_FAIL("symlink() failed: %s", strerror(errno));
        return;
    }
    passed++;

    path = path_absolute(linkpath);
    EXPECT_EQ(unlink(linkpath), 0);
    ASSERT_NONNULL(path);
    EXPECT_STREQ(path_basename(path), "README.md");
    free(path);
}

static void test_path_join(void)
{
    char *p = path_join("/", "file");
    EXPECT_STREQ(p, "/file");
    free(p);
    p = path_join("foo", "bar");
    EXPECT_STREQ(p, "foo/bar");
    free(p);
    p = path_join("foo/", "bar");
    EXPECT_STREQ(p, "foo/bar");
    free(p);
    p = path_join("", "bar");
    EXPECT_STREQ(p, "bar");
    free(p);
    p = path_join("foo", "");
    EXPECT_STREQ(p, "foo");
    free(p);
    p = path_join("", "");
    EXPECT_STREQ(p, "");
    free(p);
    p = path_join("/", "");
    EXPECT_STREQ(p, "/");
    free(p);
    p = path_join("/home/user", ".dte");
    EXPECT_STREQ(p, "/home/user/.dte");
    free(p);
    p = path_join("/home/user/", ".dte");
    EXPECT_STREQ(p, "/home/user/.dte");
    free(p);
    p = path_join("/home/user//", ".dte");
    EXPECT_STREQ(p, "/home/user//.dte");
    free(p);
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
    ssize_t size = read_file("test", &buf);
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

static void test_xfopen(void)
{
    static const char modes[][4] = {"a", "a+", "r", "r+", "w", "w+"};
    FOR_EACH_I(i, modes) {
        FILE *f = xfopen("/dev/null", modes[i], O_CLOEXEC, 0666);
        EXPECT_NONNULL(f);
        if (likely(f)) {
            EXPECT_EQ(fclose(f), 0);
        }
    }
}

static const TestEntry tests[] = {
    TEST(test_util_macros),
    TEST(test_IS_POWER_OF_2),
    TEST(test_xstreq),
    TEST(test_hex_decode),
    TEST(test_ascii),
    TEST(test_base64),
    TEST(test_string),
    TEST(test_string_view),
    TEST(test_get_delim),
    TEST(test_size_str_width),
    TEST(test_buf_parse_ulong),
    TEST(test_str_to_int),
    TEST(test_str_to_size),
    TEST(test_umax_to_str),
    TEST(test_uint_to_str),
    TEST(test_buf_uint_to_str),
    TEST(test_u_char_width),
    TEST(test_u_to_lower),
    TEST(test_u_to_upper),
    TEST(test_u_is_lower),
    TEST(test_u_is_upper),
    TEST(test_u_is_cntrl),
    TEST(test_u_is_zero_width),
    TEST(test_u_is_special_whitespace),
    TEST(test_u_is_unprintable),
    TEST(test_u_str_width),
    TEST(test_u_set_char_raw),
    TEST(test_u_set_char),
    TEST(test_u_prev_char),
    TEST(test_ptr_array),
    TEST(test_ptr_array_move),
    TEST(test_hashmap),
    TEST(test_hashset),
    TEST(test_round_size_to_next_multiple),
    TEST(test_round_size_to_next_power_of_2),
    TEST(test_path_dirname_and_path_basename),
    TEST(test_relative_filename),
    TEST(test_path_absolute),
    TEST(test_path_join),
    TEST(test_path_parent),
    TEST(test_size_multiply_overflows),
    TEST(test_size_add_overflows),
    TEST(test_mem_intern),
    TEST(test_read_file),
    TEST(test_xfopen),
};

const TestGroup util_tests = TEST_GROUP(tests);
