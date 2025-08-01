#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "test.h"
#include "util/arith.h"
#include "util/array.h"
#include "util/ascii.h"
#include "util/base64.h"
#include "util/bit.h"
#include "util/fd.h"
#include "util/fork-exec.h"
#include "util/hashmap.h"
#include "util/hashset.h"
#include "util/intern.h"
#include "util/intmap.h"
#include "util/list.h"
#include "util/log.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/progname.h"
#include "util/ptr-array.h"
#include "util/readfile.h"
#include "util/str-array.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/string.h"
#include "util/strtonum.h"
#include "util/time-util.h"
#include "util/unicode.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xmemmem.h"
#include "util/xmemrchr.h"
#include "util/xreadwrite.h"
#include "util/xsnprintf.h"
#include "util/xstdio.h"

static void test_util_macros(TestContext *ctx)
{
    EXPECT_EQ(STRLEN(""), 0);
    EXPECT_EQ(STRLEN("a"), 1);
    EXPECT_EQ(STRLEN("123456789"), 9);

    EXPECT_EQ(BITSIZE(char), 8);
    EXPECT_EQ(BITSIZE(uint16_t), 16);
    EXPECT_EQ(BITSIZE(uint32_t), 32);
    EXPECT_EQ(BITSIZE(uint64_t), 64);
    EXPECT_EQ(BITSIZE("123456789"), sizeof("123456789") * 8);

    EXPECT_EQ(HEX_STR_MAX(char), sizeof("FF") + 1);
    EXPECT_EQ(HEX_STR_MAX(uint16_t), sizeof("FFFF") + 1);
    EXPECT_EQ(HEX_STR_MAX(uint32_t), sizeof("FFFFFFFF") + 1);
    EXPECT_EQ(HEX_STR_MAX(uint64_t), sizeof("FFFFFFFFFFFFFFFF") + 1);

    EXPECT_TRUE(DECIMAL_STR_MAX(char) >= sizeof("255"));
    EXPECT_TRUE(DECIMAL_STR_MAX(uint16_t) >= sizeof("65535"));
    EXPECT_TRUE(DECIMAL_STR_MAX(uint32_t) >= sizeof("4294967295"));
    EXPECT_TRUE(DECIMAL_STR_MAX(uint64_t) >= sizeof("18446744073709551615"));

    EXPECT_EQ(ARRAYLEN(""), 1);
    EXPECT_EQ(ARRAYLEN("a"), 2);
    EXPECT_EQ(ARRAYLEN("123456789"), 10);

    UNUSED const char a2[] = {1, 2};
    UNUSED const int a3[] = {1, 2, 3};
    UNUSED const long long a4[] = {1, 2, 3, 4};
    EXPECT_EQ(ARRAYLEN(a2), 2);
    EXPECT_EQ(ARRAYLEN(a3), 3);
    EXPECT_EQ(ARRAYLEN(a4), 4);

    EXPECT_EQ(MIN(0, 1), 0);
    EXPECT_EQ(MIN(99, 100), 99);
    EXPECT_EQ(MIN(-10, 10), -10);

    EXPECT_EQ(MIN3(2, 1, 0), 0);
    EXPECT_EQ(MIN3(10, 20, 30), 10);
    EXPECT_EQ(MIN3(10, 20, -10), -10);

    EXPECT_EQ(MAX(0, 1), 1);
    EXPECT_EQ(MAX(99, 100), 100);
    EXPECT_EQ(MAX(-10, 10), 10);

    EXPECT_EQ(MAX4(1, 2, 3, 4), 4);
    EXPECT_EQ(MAX4(4, 3, 2, 1), 4);
    EXPECT_EQ(MAX4(-10, 10, 0, -20), 10);
    EXPECT_EQ(MAX4(40, 41, 42, 41), 42);
    EXPECT_EQ(MAX4(-10, -20, -50, -80), -10);

    EXPECT_EQ(CLAMP(1, 50, 100), 50);
    EXPECT_EQ(CLAMP(200, 50, 100), 100);
    EXPECT_EQ(CLAMP(200, 100, 50), 50); // Invalid edge case (lo > hi)
    EXPECT_EQ(CLAMP(-55, 50, 100), 50);
    EXPECT_EQ(CLAMP(10, -10, -20), -20); // lo > hi
    EXPECT_EQ(CLAMP(10, -20, -10), -10);
    EXPECT_EQ(CLAMP(-15, -10, -20), -20); // lo > hi
    EXPECT_EQ(CLAMP(-15, -20, -10), -15);

    EXPECT_TRUE(VERSION_GE(0, 0, 0, 0));
    EXPECT_TRUE(VERSION_GE(1, 0, 0, 0));
    EXPECT_TRUE(VERSION_GE(4, 1, 4, 1));
    EXPECT_TRUE(VERSION_GE(4, 2, 4, 1));
    EXPECT_FALSE(VERSION_GE(0, 1, 1, 0));
    EXPECT_FALSE(VERSION_GE(0, 9, 1, 0));
    EXPECT_FALSE(VERSION_GE(4, 0, 4, 1));

    EXPECT_UINT_EQ(UNSIGNED_MAX_VALUE(123U), UINT_MAX);
    EXPECT_UINT_EQ(UNSIGNED_MAX_VALUE(456UL), ULONG_MAX);
    EXPECT_UINT_EQ(UNSIGNED_MAX_VALUE(789ULL), ULLONG_MAX);
    EXPECT_UINT_EQ(UNSIGNED_MAX_VALUE((size_t)123), SIZE_MAX);
    EXPECT_UINT_EQ(UNSIGNED_MAX_VALUE((uintmax_t)456), UINTMAX_MAX);

    int n = snprintf(NULL, 0, "%d", INT_MIN);
    EXPECT_TRUE(n >= STRLEN("-2147483647"));
    EXPECT_TRUE(DECIMAL_STR_MAX(int) > n);
    n = snprintf(NULL, 0, "%llu", ULLONG_MAX);
    EXPECT_TRUE(n >= STRLEN("18446744073709551615"));
    EXPECT_TRUE(DECIMAL_STR_MAX(unsigned long long) > n);
}

static void test_is_power_of_2(TestContext *ctx)
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

    const uintmax_t max_pow2 = ~(UINTMAX_MAX >> 1);
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

// Note: some of these tests are more for the sake of AddressSanitizer
// coverage than making actual assertions about the code
static void test_xmalloc(TestContext *ctx)
{
    char *str = xmalloc(8);
    ASSERT_NONNULL(str);
    memcpy(str, "1234567", 8);
    EXPECT_STREQ(str, "1234567");
    free(str);

    str = xstrdup("foobar");
    EXPECT_STREQ(str, "foobar");
    free(str);

    str = xasprintf("%s %d", "xyz", 12340);
    EXPECT_STREQ(str, "xyz 12340");
    free(str);

    str = xcalloc(4, 1);
    ASSERT_NONNULL(str);
    EXPECT_MEMEQ(str, 4, "\0\0\0\0", 4);
    free(str);

    str = xcalloc1(4);
    ASSERT_NONNULL(str);
    EXPECT_MEMEQ(str, 4, "\0\0\0\0", 4);
    free(str);

    str = xstrslice("one two three", 4, 7);
    EXPECT_STREQ(str, "two");
    free(str);

    str = xstrjoin("foo", "-bar");
    EXPECT_STREQ(str, "foo-bar");
    free(str);

    str = xmemjoin3(STRN("123"), STRN("::"), STRN("456") + 1);
    EXPECT_STREQ(str, "123::456");
    free(str);

    str = xcalloc(4, sizeof(str[0]));
    ASSERT_NONNULL(str);
    EXPECT_EQ(str[3], 0);
    str = xrenew(str, 64);
    ASSERT_NONNULL(str);
    str[63] = 'p';
    EXPECT_EQ(str[63], 'p');
    free(str);
}

static void test_xstreq(TestContext *ctx)
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

static void test_xstrrchr(TestContext *ctx)
{
    static const char str[] = "12345432";
    EXPECT_PTREQ(xstrrchr(str, '1'), str);
    EXPECT_PTREQ(xstrrchr(str, '2'), str + 7);
    EXPECT_PTREQ(xstrrchr(str, '5'), str + 4);
    EXPECT_PTREQ(xstrrchr(str, '\0'), str + sizeof(str) - 1);
}

static void test_str_has_strn_prefix(TestContext *ctx)
{
    EXPECT_TRUE(str_has_strn_prefix("xyz", STRN("xyz")));
    EXPECT_FALSE(str_has_strn_prefix("xyz", STRN("x.z")));
    EXPECT_TRUE(str_has_strn_prefix("12345678", "1234..", 4));
    EXPECT_FALSE(str_has_strn_prefix("12345678", "1234..", 5));
    EXPECT_TRUE(str_has_strn_prefix("x", STRN("")));
    EXPECT_TRUE(str_has_strn_prefix("", STRN("")));
    EXPECT_TRUE(str_has_strn_prefix("foo", "bar", 0));
    EXPECT_FALSE(str_has_strn_prefix("foo", "bar", 3));
    EXPECT_TRUE(str_has_strn_prefix("aaa", "aaa", 4));
}

static void test_str_has_prefix(TestContext *ctx)
{
    EXPECT_TRUE(str_has_prefix("foo", "foo"));
    EXPECT_TRUE(str_has_prefix("foobar", "foo"));
    EXPECT_TRUE(str_has_prefix("xyz", "xy"));
    EXPECT_TRUE(str_has_prefix("a", "a"));
    EXPECT_FALSE(str_has_prefix("foobar", "bar"));
    EXPECT_FALSE(str_has_prefix("foo", "foobar"));
    EXPECT_FALSE(str_has_prefix("xyz", "xyz."));
    EXPECT_FALSE(str_has_prefix("ab", "b"));
    EXPECT_FALSE(str_has_prefix("123", "xyz"));
}

static void test_str_has_suffix(TestContext *ctx)
{
    EXPECT_TRUE(str_has_suffix("foo", "foo"));
    EXPECT_TRUE(str_has_suffix("foobar", "bar"));
    EXPECT_TRUE(str_has_suffix("1234", "234"));
    EXPECT_TRUE(str_has_suffix("x", "x"));
    EXPECT_TRUE(str_has_suffix("aa", "a"));
    EXPECT_FALSE(str_has_suffix("foobar.", "bar"));
    EXPECT_FALSE(str_has_suffix("foo", "foobar"));
    EXPECT_FALSE(str_has_suffix("bar", "foobar"));
    EXPECT_FALSE(str_has_suffix("foo", "bar"));
    EXPECT_FALSE(str_has_suffix("bar", "foo"));
    EXPECT_FALSE(str_has_suffix("123", "1234"));
    EXPECT_FALSE(str_has_suffix("a", "aa"));
}

static void test_hex_decode(TestContext *ctx)
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
    EXPECT_EQ(hex_decode('`'), HEX_INVALID);
    EXPECT_EQ(hex_decode('@'), HEX_INVALID);
    EXPECT_EQ(hex_decode('/'), HEX_INVALID);
    EXPECT_EQ(hex_decode(':'), HEX_INVALID);
    EXPECT_EQ(hex_decode(' '), HEX_INVALID);
    EXPECT_EQ(hex_decode('~'), HEX_INVALID);
    EXPECT_EQ(hex_decode('\0'), HEX_INVALID);
    EXPECT_EQ(hex_decode(0xFF), HEX_INVALID);

    for (unsigned int i = '0'; i <= '9'; i++) {
        IEXPECT_EQ(hex_decode(i), i - '0');
    }

    for (unsigned int i = 'A'; i <= 'F'; i++) {
        unsigned int expected = 10 + (i - 'A');
        IEXPECT_EQ(hex_decode(i), expected);
        IEXPECT_EQ(hex_decode(ascii_tolower(i)), expected);
    }

    for (unsigned int i = 0; i < 256; i++) {
        unsigned int decoded = hex_decode(i);
        bool is_hex = ascii_isalnum(i) && ascii_toupper(i) <= 'F';
        IEXPECT_EQ(decoded, is_hex ? (decoded & 0xF) : HEX_INVALID);
    }
}

static void test_hex_encode_byte(TestContext *ctx)
{
    char buf[4] = {0};
    EXPECT_EQ(hex_encode_byte(buf, 0x00), 2);
    EXPECT_STREQ(buf, "00");
    EXPECT_EQ(hex_encode_byte(buf, 0x05), 2);
    EXPECT_STREQ(buf, "05");
    EXPECT_EQ(hex_encode_byte(buf, 0x10), 2);
    EXPECT_STREQ(buf, "10");
    EXPECT_EQ(hex_encode_byte(buf, 0x1b), 2);
    EXPECT_STREQ(buf, "1b");
    EXPECT_EQ(hex_encode_byte(buf, 0xee), 2);
    EXPECT_STREQ(buf, "ee");
    EXPECT_EQ(hex_encode_byte(buf, 0xfe), 2);
    EXPECT_STREQ(buf, "fe");
    EXPECT_EQ(hex_encode_byte(buf, 0xff), 2);
    EXPECT_STREQ(buf, "ff");
}

static void test_ascii(TestContext *ctx)
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
    EXPECT_FALSE(ascii_isspace('\v')); // (differs from POSIX)
    EXPECT_FALSE(ascii_isspace('\f')); // (differs from POSIX)
    EXPECT_FALSE(ascii_isspace('\0'));
    EXPECT_FALSE(ascii_isspace('a'));
    EXPECT_FALSE(ascii_isspace(0x7F));
    EXPECT_FALSE(ascii_isspace(0x80));

    // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html#tag_07_03_01_01:~:text=cntrl,-%3Calert
    // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html#tag_07_03_01_01:~:text=may%20add%20additional%20characters
    // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap06.html
    EXPECT_TRUE(ascii_iscntrl('\0')); // <NUL>
    EXPECT_TRUE(ascii_iscntrl('\a')); // <alert>, <BEL>
    EXPECT_TRUE(ascii_iscntrl('\b')); // <backspace>, <BS>
    EXPECT_TRUE(ascii_iscntrl('\t')); // <tab>, <HT>
    EXPECT_TRUE(ascii_iscntrl('\n')); // <newline>, <LF>
    EXPECT_TRUE(ascii_iscntrl('\v')); // <vertical-tab>, <VT>
    EXPECT_TRUE(ascii_iscntrl('\f')); // <form-feed>, <FF>
    EXPECT_TRUE(ascii_iscntrl('\r')); // <carriage-return>, <CR>
    EXPECT_TRUE(ascii_iscntrl(0x0E)); // <SO>
    EXPECT_TRUE(ascii_iscntrl(0x1F)); // <IS1>, <US>
    EXPECT_TRUE(ascii_iscntrl(0x7F)); // <DEL>
    EXPECT_FALSE(ascii_iscntrl(' ')); // <space>
    EXPECT_FALSE(ascii_iscntrl('a')); // <a>
    EXPECT_FALSE(ascii_iscntrl('~')); // <tilde>
    EXPECT_FALSE(ascii_iscntrl(0x80));
    EXPECT_FALSE(ascii_iscntrl(0xFF));

    // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html#tag_07_03_01_01:~:text=cntrl,-%3Calert
    // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html#tag_07_03_01_01:~:text=space,-%3Ctab
    // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html#tag_07_03_01_01:~:text=may%20add%20additional%20characters
    // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap06.html
    EXPECT_TRUE(ascii_is_nonspace_cntrl('\0')); // <NUL>
    EXPECT_TRUE(ascii_is_nonspace_cntrl('\a')); // <alert>, <BEL>
    EXPECT_TRUE(ascii_is_nonspace_cntrl('\b')); // <backspace>, <BS>
    EXPECT_TRUE(ascii_is_nonspace_cntrl(0x0E)); // <SO>
    EXPECT_TRUE(ascii_is_nonspace_cntrl(0x1F)); // <IS1>, <US>
    EXPECT_TRUE(ascii_is_nonspace_cntrl(0x7F)); // <DEL>
    EXPECT_TRUE(ascii_is_nonspace_cntrl('\v')); // <vertical-tab>, <VT> (differs from POSIX)
    EXPECT_TRUE(ascii_is_nonspace_cntrl('\f')); // <form-feed>, <FF> (differs from POSIX)
    EXPECT_FALSE(ascii_is_nonspace_cntrl('\t')); // <tab>, <HT>
    EXPECT_FALSE(ascii_is_nonspace_cntrl('\n')); // <newline>, <LF>
    EXPECT_FALSE(ascii_is_nonspace_cntrl('\r')); // <carriage-return>, <CR>
    EXPECT_FALSE(ascii_is_nonspace_cntrl(' ')); // <space>
    EXPECT_FALSE(ascii_is_nonspace_cntrl('a'));
    EXPECT_FALSE(ascii_is_nonspace_cntrl(0x7E));
    EXPECT_FALSE(ascii_is_nonspace_cntrl(0x80));
    EXPECT_FALSE(ascii_is_nonspace_cntrl(0xFF));

    // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html#tag_07_03_01_01:~:text=punct,-%3Cexclamation
    // https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html#tag_07_03_01_01:~:text=may%20add%20additional%20characters
    EXPECT_TRUE(ascii_ispunct('!'));
    EXPECT_TRUE(ascii_ispunct('"'));
    EXPECT_TRUE(ascii_ispunct('#'));
    EXPECT_TRUE(ascii_ispunct('$'));
    EXPECT_TRUE(ascii_ispunct('%'));
    EXPECT_TRUE(ascii_ispunct('&'));
    EXPECT_TRUE(ascii_ispunct('\''));
    EXPECT_TRUE(ascii_ispunct('('));
    EXPECT_TRUE(ascii_ispunct(')'));
    EXPECT_TRUE(ascii_ispunct('*'));
    EXPECT_TRUE(ascii_ispunct('+'));
    EXPECT_TRUE(ascii_ispunct(','));
    EXPECT_TRUE(ascii_ispunct('-'));
    EXPECT_TRUE(ascii_ispunct('.'));
    EXPECT_TRUE(ascii_ispunct('/'));
    EXPECT_TRUE(ascii_ispunct(':'));
    EXPECT_TRUE(ascii_ispunct(';'));
    EXPECT_TRUE(ascii_ispunct('<'));
    EXPECT_TRUE(ascii_ispunct('='));
    EXPECT_TRUE(ascii_ispunct('>'));
    EXPECT_TRUE(ascii_ispunct('?'));
    EXPECT_TRUE(ascii_ispunct('@'));
    EXPECT_TRUE(ascii_ispunct('['));
    EXPECT_TRUE(ascii_ispunct('\\'));
    EXPECT_TRUE(ascii_ispunct(']'));
    EXPECT_TRUE(ascii_ispunct('^'));
    EXPECT_TRUE(ascii_ispunct('_'));
    EXPECT_TRUE(ascii_ispunct('`'));
    EXPECT_TRUE(ascii_ispunct('{'));
    EXPECT_TRUE(ascii_ispunct('|'));
    EXPECT_TRUE(ascii_ispunct('}'));
    EXPECT_TRUE(ascii_ispunct('~'));
    EXPECT_FALSE(ascii_ispunct(' '));
    EXPECT_FALSE(ascii_ispunct('0'));
    EXPECT_FALSE(ascii_ispunct('9'));
    EXPECT_FALSE(ascii_ispunct('A'));
    EXPECT_FALSE(ascii_ispunct('Z'));
    EXPECT_FALSE(ascii_ispunct('a'));
    EXPECT_FALSE(ascii_ispunct('z'));
    EXPECT_FALSE(ascii_ispunct(0x00));
    EXPECT_FALSE(ascii_ispunct(0x7F));
    EXPECT_FALSE(ascii_ispunct(0xFF));

    EXPECT_TRUE(ascii_isdigit('0'));
    EXPECT_TRUE(ascii_isdigit('1'));
    EXPECT_TRUE(ascii_isdigit('9'));
    EXPECT_FALSE(ascii_isdigit('a'));
    EXPECT_FALSE(ascii_isdigit('f'));
    EXPECT_FALSE(ascii_isdigit('/'));
    EXPECT_FALSE(ascii_isdigit(':'));
    EXPECT_FALSE(ascii_isdigit('\0'));
    EXPECT_FALSE(ascii_isdigit(0xFF));

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

    EXPECT_TRUE(is_regex_special_char('$'));
    EXPECT_TRUE(is_regex_special_char('('));
    EXPECT_TRUE(is_regex_special_char(')'));
    EXPECT_TRUE(is_regex_special_char('*'));
    EXPECT_TRUE(is_regex_special_char('+'));
    EXPECT_TRUE(is_regex_special_char('.'));
    EXPECT_TRUE(is_regex_special_char('?'));
    EXPECT_TRUE(is_regex_special_char('['));
    EXPECT_TRUE(is_regex_special_char('^'));
    EXPECT_TRUE(is_regex_special_char('{'));
    EXPECT_TRUE(is_regex_special_char('|'));
    EXPECT_TRUE(is_regex_special_char('\\'));
    EXPECT_FALSE(is_regex_special_char('"'));
    EXPECT_FALSE(is_regex_special_char('&'));
    EXPECT_FALSE(is_regex_special_char(','));
    EXPECT_FALSE(is_regex_special_char('0'));
    EXPECT_FALSE(is_regex_special_char('@'));
    EXPECT_FALSE(is_regex_special_char('A'));
    EXPECT_FALSE(is_regex_special_char('\''));
    EXPECT_FALSE(is_regex_special_char(']'));
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

    EXPECT_EQ(ascii_strcmp_icase("", ""), 0);
    EXPECT_EQ(ascii_strcmp_icase("A", "A"), 0);
    EXPECT_EQ(ascii_strcmp_icase("xyz", ""), 'x');
    EXPECT_EQ(ascii_strcmp_icase("", "xyz"), -'x');
    EXPECT_EQ(ascii_strcmp_icase("xyz", "xy"), 'z');
    EXPECT_EQ(ascii_strcmp_icase("xy", "xyz"), -'z');
    EXPECT_EQ(ascii_strcmp_icase("\xFF", "\xFE"), 1);
    EXPECT_EQ(ascii_strcmp_icase("\xFE", "\xFF"), -1);
    EXPECT_EQ(ascii_strcmp_icase("\x80\xFF\xC1", "\x80\xFF\x01"), 0xC0);
    EXPECT_EQ(ascii_strcmp_icase("\x80\xFF\x01", "\x80\xFF\xC1"), -0xC0);
    EXPECT_EQ(ascii_strcmp_icase("\x80\xFF\x01", "\x80"), 0xFF);
    EXPECT_EQ(ascii_strcmp_icase("\x80", "\x80\xFF\x01"), -0xFF);

    // Query the current locale
    const char *locale = setlocale(LC_CTYPE, NULL);
    ASSERT_NONNULL(locale);

    // Copy the locale string (which may be in static storage)
    char *saved_locale = xstrdup(locale);

    // Check that the ascii_is*() functions behave like their corresponding
    // <ctype.h> macros, when in the standard C/POSIX locale.
    // See also: https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html#tag_07_03_01_01
    ASSERT_NONNULL(setlocale(LC_CTYPE, "C"));
    for (int i = -1; i < 256; i++) {
        EXPECT_EQ(ascii_isalpha(i), !!isalpha(i));
        EXPECT_EQ(ascii_isalnum(i), !!isalnum(i));
        EXPECT_EQ(ascii_islower(i), !!islower(i));
        EXPECT_EQ(ascii_isupper(i), !!isupper(i));
        EXPECT_EQ(ascii_isdigit(i), !!isdigit(i));
        EXPECT_EQ(ascii_isblank(i), !!isblank(i));
        EXPECT_EQ(ascii_isprint(i), !!isprint(i));
        EXPECT_EQ(ascii_isxdigit(i), !!isxdigit(i));
        EXPECT_EQ(u_is_ascii_upper(i), !!isupper(i));
        EXPECT_EQ(is_alpha_or_underscore(i), !!isalpha(i) || i == '_');
        EXPECT_EQ(is_alnum_or_underscore(i), !!isalnum(i) || i == '_');
        if (i != '\v' && i != '\f') {
            EXPECT_EQ(ascii_isspace(i), !!isspace(i));
        }
        if (i != -1) {
            EXPECT_EQ(is_word_byte(i), !!isalnum(i) || i == '_' || i >= 0x80);
            EXPECT_EQ(ascii_tolower(i), tolower(i));
            EXPECT_EQ(ascii_toupper(i), toupper(i));
        }
    }

    // Restore the original locale
    ASSERT_NONNULL(setlocale(LC_CTYPE, saved_locale));
    free(saved_locale);
}

static void test_mem_equal(TestContext *ctx)
{
    static const char s1[] = "abcxyz";
    static const char s2[] = "abcXYZ";
    EXPECT_TRUE(mem_equal(NULL, NULL, 0));
    EXPECT_TRUE(mem_equal(s1, s2, 0));
    EXPECT_TRUE(mem_equal(s1, s2, 1));
    EXPECT_TRUE(mem_equal(s1, s2, 2));
    EXPECT_TRUE(mem_equal(s1, s2, 3));
    EXPECT_TRUE(mem_equal(s1 + 1, s2 + 1, 2));
    EXPECT_TRUE(mem_equal(s1 + 6, s2 + 6, 1));
    EXPECT_FALSE(mem_equal(s1, s2, 4));
    EXPECT_FALSE(mem_equal(s1, s2, 5));
    EXPECT_FALSE(mem_equal(s1, s2, 6));
    EXPECT_FALSE(mem_equal(s1, s2, 7));
}

static void test_mem_equal_icase(TestContext *ctx)
{
    static const char s1[8] = "Ctrl+Up";
    static const char s2[8] = "CTRL+U_";
    EXPECT_TRUE(mem_equal_icase(NULL, NULL, 0));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 0));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 1));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 2));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 3));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 4));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 5));
    EXPECT_TRUE(mem_equal_icase(s1, s2, 6));
    EXPECT_FALSE(mem_equal_icase(s1, s2, 7));
    EXPECT_FALSE(mem_equal_icase(s1, s2, 8));
}

static void test_base64_decode(TestContext *ctx)
{
    EXPECT_EQ(base64_decode('A'), 0);
    EXPECT_EQ(base64_decode('Z'), 25);
    EXPECT_EQ(base64_decode('a'), 26);
    EXPECT_EQ(base64_decode('z'), 51);
    EXPECT_EQ(base64_decode('0'), 52);
    EXPECT_EQ(base64_decode('9'), 61);
    EXPECT_EQ(base64_decode('+'), 62);
    EXPECT_EQ(base64_decode('/'), 63);

    EXPECT_EQ(base64_encode_table[0], 'A');
    EXPECT_EQ(base64_encode_table[25], 'Z');
    EXPECT_EQ(base64_encode_table[26], 'a');
    EXPECT_EQ(base64_encode_table[51], 'z');
    EXPECT_EQ(base64_encode_table[52], '0');
    EXPECT_EQ(base64_encode_table[61], '9');
    EXPECT_EQ(base64_encode_table[62], '+');
    EXPECT_EQ(base64_encode_table[63], '/');

    EXPECT_EQ(base64_decode('='), BASE64_PADDING);
    EXPECT_EQ(base64_decode(' '), BASE64_INVALID);
    EXPECT_EQ(base64_decode('*'), BASE64_INVALID);
    EXPECT_EQ(base64_decode(','), BASE64_INVALID);
    EXPECT_EQ(base64_decode(':'), BASE64_INVALID);
    EXPECT_EQ(base64_decode('?'), BASE64_INVALID);
    EXPECT_EQ(base64_decode('@'), BASE64_INVALID);
    EXPECT_EQ(base64_decode('['), BASE64_INVALID);
    EXPECT_EQ(base64_decode('`'), BASE64_INVALID);
    EXPECT_EQ(base64_decode('{'), BASE64_INVALID);
    EXPECT_EQ(base64_decode('~'), BASE64_INVALID);
    EXPECT_EQ(base64_decode('}'), BASE64_INVALID);
    EXPECT_EQ(base64_decode(0), BASE64_INVALID);
    EXPECT_EQ(base64_decode(127), BASE64_INVALID);
    EXPECT_EQ(base64_decode(128), BASE64_INVALID);
    EXPECT_EQ(base64_decode(255), BASE64_INVALID);

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
            IEXPECT_EQ(i, base64_encode_table[val & 63]);
        } else {
            IEXPECT_EQ(val, val & 192);
        }
        IEXPECT_EQ(val, base64_decode_branchy(i));
    }
}

static void test_base64_encode_block(TestContext *ctx)
{
    char buf[16];
    size_t n = base64_encode_block(STRN("xyz"), buf, sizeof(buf));
    EXPECT_MEMEQ(buf, n, "eHl6", 4);

    n = base64_encode_block(STRN("123456"), buf, sizeof(buf));
    EXPECT_MEMEQ(buf, n, "MTIzNDU2", 8);

    n = base64_encode_block(STRN("a == *x++"), buf, sizeof(buf));
    EXPECT_MEMEQ(buf, n, "YSA9PSAqeCsr", 12);
}

static void test_base64_encode_final(TestContext *ctx)
{
    char buf[4];
    base64_encode_final(STRN("+"), buf);
    EXPECT_MEMEQ(buf, 4, "Kw==", 4);

    base64_encode_final(STRN(".."), buf);
    EXPECT_MEMEQ(buf, 4, "Li4=", 4);

    base64_encode_final(STRN("~."), buf);
    EXPECT_MEMEQ(buf, 4, "fi4=", 4);

    base64_encode_final(STRN("\xC2\xA9"), buf);
    EXPECT_MEMEQ(buf, 4, "wqk=", 4);
}

static void test_string(TestContext *ctx)
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

    EXPECT_EQ(string_insert_codepoint(&s, 0, 0x1F4AF), 4);
    EXPECT_EQ(s.len, 4);
    EXPECT_STREQ(string_borrow_cstring(&s), "\xF0\x9F\x92\xAF");

    string_append_cstring(&s, "test");
    EXPECT_EQ(s.len, 8);
    EXPECT_STREQ(string_borrow_cstring(&s), "\xF0\x9F\x92\xAFtest");

    string_remove(&s, 0, 5);
    EXPECT_EQ(s.len, 3);
    EXPECT_STREQ(string_borrow_cstring(&s), "est");

    EXPECT_EQ(string_insert_codepoint(&s, 0, 't'), 1);
    EXPECT_EQ(s.len, 4);
    EXPECT_STREQ(string_borrow_cstring(&s), "test");

    EXPECT_EQ(string_clear(&s), 4);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(string_insert_codepoint(&s, 0, 0x0E01), 3);
    EXPECT_EQ(s.len, 3);
    EXPECT_STREQ(string_borrow_cstring(&s), "\xE0\xB8\x81");

    EXPECT_EQ(string_clear(&s), 3);
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

    EXPECT_EQ(string_insert_codepoint(&s, 0, '>'), 1);
    EXPECT_STREQ(string_borrow_cstring(&s), ">12foo3123");
    EXPECT_EQ(s.len, 10);

    string_replace_byte(&s, '1', '_');
    EXPECT_STREQ(string_borrow_cstring(&s), ">_2foo3_23");
    string_replace_byte(&s, '_', '.');
    string_replace_byte(&s, '2', '+');
    string_replace_byte(&s, '3', '$');
    EXPECT_STREQ(string_borrow_cstring(&s), ">.+foo$.+$");

    string_free(&s);
    EXPECT_NULL(s.buffer);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    EXPECT_STREQ(cstr, "12foo3123");
    free(cstr);

    // This is mostly for UBSan coverage
    // See also: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3322.pdf
    s = string_new(0);
    EXPECT_NULL(s.buffer);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    string_append_buf(&s, NULL, 0);
    string_insert_buf(&s, 0, NULL, 0);
    string_append_memset(&s, 'q', 0);
    string_replace_byte(&s, 'q', 'z');
    string_remove(&s, 0, 0);
    EXPECT_NULL(s.buffer);
    EXPECT_EQ(s.len, 0);
    EXPECT_EQ(s.alloc, 0);
    string_free(&s);

    s = string_new_from_buf(STRN("1234567"));
    EXPECT_EQ(s.len, 7);
    EXPECT_EQ(s.alloc, 16);
    EXPECT_STREQ(string_borrow_cstring(&s), "1234567");
    string_free(&s);
}

static void test_string_view(TestContext *ctx)
{
    StringView sv = STRING_VIEW("testing");
    EXPECT_TRUE(strview_equal_cstring(&sv, "testing"));
    EXPECT_FALSE(strview_equal_cstring(&sv, "testin"));
    EXPECT_FALSE(strview_equal_cstring(&sv, "TESTING"));
    EXPECT_TRUE(strview_has_prefix(&sv, "test"));
    EXPECT_TRUE(strview_has_prefix_icase(&sv, "TEst"));
    EXPECT_FALSE(strview_has_prefix(&sv, "TEst"));
    EXPECT_FALSE(strview_has_prefix_icase(&sv, "TEst_"));

    sv = string_view(sv.data, sv.length);
    EXPECT_TRUE(strview_equal(&sv, &sv));
    sv = string_view(STRN("\0test\0 ..."));
    EXPECT_TRUE(strview_equal_strn(&sv, "\0test\0 ...", 10));

    sv = strview_from_cstring("foobar");
    EXPECT_TRUE(strview_equal_cstring(&sv, "foobar"));
    EXPECT_TRUE(strview_has_prefix(&sv, "foo"));
    EXPECT_FALSE(strview_equal_cstring(&sv, "foo"));

    sv = strview_from_cstring("\t  \t\t ");
    EXPECT_TRUE(strview_isblank(&sv));
    sv.length = 0;
    EXPECT_TRUE(strview_isblank(&sv));
    sv = strview_from_cstring("    \t .  ");
    EXPECT_FALSE(strview_isblank(&sv));
    sv = strview_from_cstring("\n");
    EXPECT_FALSE(strview_isblank(&sv));
    sv = strview_from_cstring("  \r ");
    EXPECT_FALSE(strview_isblank(&sv));

    sv = strview_from_cstring("  \t\t \ttrim test \t\t");
    strview_trim(&sv);
    EXPECT_TRUE(strview_equal_cstring(&sv, "trim test"));

    sv = string_view(NULL, 0);
    EXPECT_NULL(strview_memrchr(&sv, '.'));
    EXPECT_NULL(strview_memchr(&sv, '.'));
    EXPECT_TRUE(strview_equal(&sv, &sv));
    EXPECT_FALSE(strview_contains_char_type(&sv, ASCII_DIGIT));
    EXPECT_TRUE(strview_isblank(&sv));
    EXPECT_EQ(strview_trim_left(&sv), 0);
    EXPECT_TRUE(strview_equal_cstring(&sv, ""));
    EXPECT_TRUE(strview_equal_cstring_icase(&sv, ""));
    EXPECT_TRUE(strview_has_prefix(&sv, ""));
    EXPECT_TRUE(strview_has_suffix(&sv, ""));
    EXPECT_TRUE(strview_has_prefix_icase(&sv, ""));

    // Call these 3 for ASan/UBSan coverage:
    strview_trim_right(&sv);
    strview_trim(&sv);
    strview_remove_prefix(&sv, 0);
    EXPECT_NULL(sv.data);
    EXPECT_EQ(sv.length, 0);

    sv = strview_from_cstring("prefix - suffix");
    EXPECT_PTREQ(strview_memchr(&sv, '-'), strview_memrchr(&sv, '-'));
    EXPECT_PTREQ(strview_memchr(&sv, 'x'), sv.data + 5);
    EXPECT_PTREQ(strview_memrchr(&sv, 'x'), sv.data + 14);
    EXPECT_PTREQ(strview_memrchr(&sv, '@'), NULL);
    EXPECT_EQ(strview_memrchr_idx(&sv, 'p'), 0);
    EXPECT_EQ(strview_memrchr_idx(&sv, 'x'), 14);
    EXPECT_EQ(strview_memrchr_idx(&sv, '@'), -1);
}

static void test_strview_has_suffix(TestContext *ctx)
{
    StringView sv = strview_from_cstring("foobar");
    EXPECT_TRUE(strview_has_suffix(&sv, "foobar"));
    EXPECT_TRUE(strview_has_suffix(&sv, "bar"));
    EXPECT_TRUE(strview_has_suffix(&sv, "r"));
    EXPECT_TRUE(strview_has_suffix(&sv, ""));
    EXPECT_FALSE(strview_has_suffix(&sv, "foo"));
    EXPECT_FALSE(strview_has_suffix(&sv, "foobars"));

    const StringView suffix = string_view(NULL, 0);
    EXPECT_TRUE(strview_has_sv_suffix(sv, suffix));
    EXPECT_TRUE(strview_has_sv_suffix(suffix, suffix));

    sv.length--;
    EXPECT_FALSE(strview_has_suffix(&sv, "bar"));
    EXPECT_TRUE(strview_has_suffix(&sv, "ba"));
    EXPECT_TRUE(strview_has_sv_suffix(sv, suffix));

    sv.length = 0;
    EXPECT_TRUE(strview_has_suffix(&sv, ""));
    EXPECT_FALSE(strview_has_suffix(&sv, "f"));
    EXPECT_TRUE(strview_has_sv_suffix(sv, suffix));

    sv.data = NULL;
    EXPECT_TRUE(strview_has_suffix(&sv, ""));
    EXPECT_FALSE(strview_has_suffix(&sv, "f"));
    EXPECT_TRUE(strview_has_sv_suffix(sv, suffix));
}

static void test_strview_remove_matching(TestContext *ctx)
{
    StringView sv = STRING_VIEW("ABCDEFGHIJKLMN");
    EXPECT_TRUE(strview_remove_matching_prefix(&sv, "ABC"));
    EXPECT_STRVIEW_EQ_CSTRING(&sv, "DEFGHIJKLMN");

    EXPECT_TRUE(strview_remove_matching_suffix(&sv, "KLMN"));
    EXPECT_STRVIEW_EQ_CSTRING(&sv, "DEFGHIJ");

    EXPECT_FALSE(strview_remove_matching_prefix(&sv, "A"));
    EXPECT_FALSE(strview_remove_matching_suffix(&sv, "K"));
    EXPECT_STRVIEW_EQ_CSTRING(&sv, "DEFGHIJ");

    EXPECT_TRUE(strview_remove_matching_prefix(&sv, ""));
    EXPECT_STRVIEW_EQ_CSTRING(&sv, "DEFGHIJ");

    EXPECT_TRUE(strview_remove_matching_suffix(&sv, ""));
    EXPECT_STRVIEW_EQ_CSTRING(&sv, "DEFGHIJ");

    sv.length = 0;
    sv.data = NULL;
    EXPECT_TRUE(strview_remove_matching_prefix(&sv, ""));
    EXPECT_TRUE(strview_remove_matching_suffix(&sv, ""));
    EXPECT_FALSE(strview_remove_matching_prefix(&sv, "pre"));
    EXPECT_FALSE(strview_remove_matching_suffix(&sv, "suf"));
    EXPECT_EQ(sv.length, 0);
    EXPECT_NULL(sv.data);
    EXPECT_STRVIEW_EQ_CSTRING(&sv, "");
}

static void test_get_delim(TestContext *ctx)
{
    static const char input[] = "-x-y-foo--bar--";
    static const char parts[][4] = {"", "x", "y", "foo", "", "bar", "", ""};
    const size_t nparts = ARRAYLEN(parts);
    const size_t part_size = ARRAYLEN(parts[0]);

    size_t idx = 0;
    for (size_t pos = 0, len = sizeof(input) - 1; pos < len; idx++) {
        const StringView sv = get_delim(input, &pos, len, '-');
        ASSERT_TRUE(idx < nparts);
        ASSERT_EQ(parts[idx][part_size - 1], '\0');
        EXPECT_STRVIEW_EQ_CSTRING(&sv, parts[idx]);
    }

    EXPECT_EQ(idx, nparts - 1);
}

static void test_get_delim_str(TestContext *ctx)
{
    char str[] = "word1-word2-end-";
    size_t len = sizeof(str) - 1;
    ASSERT_EQ(str[len - 1], '-'); // Last character in bounds is a delimiter

    size_t pos = 0;
    const char *substr = get_delim_str(str, &pos, len, '-');
    EXPECT_STREQ(substr, "word1");
    EXPECT_EQ(pos, 6);

    substr = get_delim_str(str, &pos, len, '-');
    EXPECT_STREQ(substr, "word2");
    EXPECT_EQ(pos, 12);

    substr = get_delim_str(str, &pos, len, '-');
    EXPECT_STREQ(substr, "end");
    EXPECT_EQ(pos, 16);

    // Note: str2 is not null-terminated and there are no delimiters present,
    // but there's one extra, writable byte in the buffer that falls outside
    // the length bound
    char str2[16] = "no delimiter...!";
    len = sizeof(str2) - 1;
    ASSERT_EQ(str2[len], '!');
    pos = 0;
    substr = get_delim_str(str2, &pos, len, '-');
    EXPECT_STREQ(substr, "no delimiter...");
    EXPECT_EQ(pos, len);

    static const char after[] = "\0\0\0\0aa\0b\0c\0dd\0\0";
    char before[] = ",,,,aa,b,c,dd,,";
    unsigned int iters = 0;
    unsigned int width = 0;
    for (pos = 0, len = sizeof(before) - 1; pos < len; iters++) {
        width += strlen(get_delim_str(before, &pos, len, ','));
    }
    EXPECT_MEMEQ(before, sizeof(before), after, sizeof(after));
    EXPECT_EQ(width, 6);
    EXPECT_EQ(iters, 9);
}

static void test_str_replace_byte(TestContext *ctx)
{
    char str[] = " a b c  d e  f g ";
    EXPECT_EQ(str_replace_byte(str, ' ', '-'), strlen(str));
    EXPECT_STREQ(str, "-a-b-c--d-e--f-g-");
}

static void test_strn_replace_byte(TestContext *ctx)
{
    static const char expected[] = "||a|b|c||\n\0\0|d|e|f|||\0g|h||\0\0";
    char str[] = /* ........... */ "..a.b.c..\n\0\0.d.e.f...\0g.h..\0\0";
    strn_replace_byte(str, sizeof(str), '.', '|');
    EXPECT_MEMEQ(str, sizeof(str), expected, sizeof(expected));
}

static void test_string_array_concat(TestContext *ctx)
{
    static const char *const strs[] = {"A", "B", "CC", "D", "EE"};
    char buf[64] = "\0";
    size_t nstrs = ARRAYLEN(strs);

    const char *delim = ",";
    const char *str = "A,B,CC,D,EE";
    size_t delim_len = strlen(delim);
    size_t n = strlen(str);
    ASSERT_TRUE(n + 1 < sizeof(buf));
    memset(buf, '@', sizeof(buf) - 1);
    EXPECT_FALSE(string_array_concat_(buf, n, strs, nstrs, delim, delim_len));
    memset(buf, '@', sizeof(buf) - 1);
    EXPECT_TRUE(string_array_concat_(buf, n + 1, strs, nstrs, delim, delim_len));
    EXPECT_STREQ(buf, str);

    delim = " ... ";
    delim_len = strlen(delim);
    str = "A ... B ... CC ... D ... EE";
    n = strlen(str);
    ASSERT_TRUE(n + 1 < sizeof(buf));
    memset(buf, '@', sizeof(buf) - 1);
    EXPECT_FALSE(string_array_concat_(buf, n, strs, nstrs, delim, delim_len));
    memset(buf, '@', sizeof(buf) - 1);
    EXPECT_TRUE(string_array_concat_(buf, n + 1, strs, nstrs, delim, delim_len));
    EXPECT_STREQ(buf, str);

    for (size_t i = 0; i < n; i++) {
        EXPECT_FALSE(string_array_concat_(buf, n - i, strs, nstrs, delim, delim_len));
    }

    memset(buf, '@', sizeof(buf) - 1);
    EXPECT_TRUE(string_array_concat_(buf, sizeof(buf), strs, 0, delim, delim_len));
    EXPECT_STREQ(buf, "");
}

static void test_size_str_width(TestContext *ctx)
{
    EXPECT_EQ(size_str_width(0), 1);
    EXPECT_EQ(size_str_width(1), 1);
    EXPECT_EQ(size_str_width(9), 1);
    EXPECT_EQ(size_str_width(19), 2);
    EXPECT_EQ(size_str_width(425), 3);
    EXPECT_EQ(size_str_width(12345), 5);
    EXPECT_EQ(size_str_width(2147483647), 10);
}

static void test_buf_parse_uintmax(TestContext *ctx)
{
    uintmax_t val;
    char max[DECIMAL_STR_MAX(val) + 2];
    size_t max_len = xsnprintf(max, sizeof max, "%ju", UINTMAX_MAX);

    val = 11;
    EXPECT_EQ(buf_parse_uintmax(max, max_len, &val), max_len);
    EXPECT_UINT_EQ(val, UINTMAX_MAX);

    val = 22;
    max[max_len++] = '9';
    EXPECT_EQ(buf_parse_uintmax(max, max_len, &val), 0);
    EXPECT_EQ(val, 22);

    val = 33;
    max[max_len++] = '7';
    EXPECT_EQ(buf_parse_uintmax(max, max_len, &val), 0);
    EXPECT_EQ(val, 33);

    EXPECT_EQ(buf_parse_uintmax("0", 1, &val), 1);
    EXPECT_EQ(val, 0);

    EXPECT_EQ(buf_parse_uintmax("0019817", 8, &val), 7);
    EXPECT_EQ(val, 19817);

    EXPECT_EQ(buf_parse_uintmax("0098765", 5, &val), 5);
    EXPECT_EQ(val, 987);

    char buf[4] = " 90/";
    buf[0] = CHAR_MAX;
    EXPECT_EQ(buf_parse_uintmax(buf, 4, &val), 0);

    for (char c = CHAR_MIN; c < CHAR_MAX; c++) {
        buf[0] = c;
        if (!ascii_isdigit(c)) {
            EXPECT_EQ(buf_parse_uintmax(buf, 4, &val), 0);
            continue;
        }
        val = 337;
        EXPECT_EQ(buf_parse_uintmax(buf, 0, &val), 0);
        EXPECT_EQ(val, 337);
        EXPECT_EQ(buf_parse_uintmax(buf, 1, &val), 1);
        EXPECT_TRUE(val <= 9);
        EXPECT_EQ(buf_parse_uintmax(buf, 2, &val), 2);
        EXPECT_TRUE(val >= 9);
        EXPECT_TRUE(val <= 99);
        EXPECT_EQ(buf_parse_uintmax(buf, 3, &val), 3);
        EXPECT_TRUE(val >= 90);
        EXPECT_TRUE(val <= 990);
        const uintmax_t prev = val;
        EXPECT_EQ(buf_parse_uintmax(buf, 4, &val), 3);
        EXPECT_EQ(val, prev);
    }
}

static void test_buf_parse_ulong(TestContext *ctx)
{
    unsigned long val;
    char max[DECIMAL_STR_MAX(val) + 1];
    size_t max_len = xsnprintf(max, sizeof max, "%lu", ULONG_MAX);

    val = 88;
    EXPECT_EQ(buf_parse_ulong(max, max_len, &val), max_len);
    EXPECT_UINT_EQ(val, ULONG_MAX);

    val = 99;
    max[max_len++] = '1';
    EXPECT_EQ(buf_parse_ulong(max, max_len, &val), 0);
    EXPECT_EQ(val, 99);

    EXPECT_EQ(buf_parse_ulong("0", 1, &val), 1);
    EXPECT_EQ(val, 0);

    EXPECT_EQ(buf_parse_ulong("9876", 4, &val), 4);
    EXPECT_EQ(val, 9876);
}

static void test_buf_parse_size(TestContext *ctx)
{
    size_t val;
    char max[DECIMAL_STR_MAX(val) + 1];
    size_t max_len = xsnprintf(max, sizeof max, "%zu", SIZE_MAX);

    val = 14;
    EXPECT_EQ(buf_parse_size(max, max_len, &val), max_len);
    EXPECT_UINT_EQ(val, SIZE_MAX);

    val = 88;
    max[max_len++] = '0';
    EXPECT_EQ(buf_parse_size(max, max_len, &val), 0);
    EXPECT_EQ(val, 88);
}

static void test_buf_parse_hex_uint(TestContext *ctx)
{
    unsigned int val;
    char buf[HEX_STR_MAX(val) + 1];
    size_t buf_len = xsnprintf(buf, sizeof buf, "%x", UINT_MAX);
    val = 0x90;
    EXPECT_EQ(buf_parse_hex_uint(buf, buf_len, &val), buf_len);
    EXPECT_UINT_EQ(val, UINT_MAX);

    buf_len = xsnprintf(buf, sizeof buf, "1%x", UINT_MAX);
    val = 0x100;
    EXPECT_EQ(buf_parse_hex_uint(buf, buf_len, &val), 0);
    EXPECT_UINT_EQ(val, 0x100);

    buf_len = xsnprintf(buf, sizeof buf, "%xg", UINT_MAX);
    val = 0x110;
    EXPECT_EQ(buf_parse_hex_uint(buf, buf_len, &val), buf_len - 1);
    EXPECT_UINT_EQ(val, UINT_MAX);

    buf_len = xsnprintf(buf, sizeof buf, "%xf", UINT_MAX);
    val = 0x120;
    EXPECT_EQ(buf_parse_hex_uint(buf, buf_len, &val), 0);
    EXPECT_UINT_EQ(val, 0x120);

    buf_len = sizeof(buf);
    memset(buf, '0', buf_len);
    val = 0x130;
    EXPECT_EQ(buf_parse_hex_uint(buf, buf_len, &val), buf_len);
    EXPECT_UINT_EQ(val, 0);

    val = 0x140;
    EXPECT_EQ(buf_parse_hex_uint(buf, buf_len - 2, &val), buf_len - 2);
    EXPECT_UINT_EQ(val, 0);

    val = 0x150;
    EXPECT_EQ(buf_parse_hex_uint(STRN("12345678"), &val), 8);
    EXPECT_UINT_EQ(val, 0x12345678);

    val = 0x160;
    EXPECT_EQ(buf_parse_hex_uint(STRN("00abcdeF01"), &val), 10);
    EXPECT_UINT_EQ(val, 0xABCDEF01);

    val = 0x170;
    EXPECT_EQ(buf_parse_hex_uint("0123456789", 4, &val), 4);
    EXPECT_UINT_EQ(val, 0x123);
}

static void test_str_to_int(TestContext *ctx)
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

static void test_str_to_size(TestContext *ctx)
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

static void test_str_to_filepos(TestContext *ctx)
{
    size_t line = 0;
    size_t col = 0;
    EXPECT_TRUE(str_to_filepos("10,60", &line, &col));
    EXPECT_EQ(line, 10);
    EXPECT_EQ(col, 60);

    EXPECT_TRUE(str_to_filepos("1:9", &line, &col));
    EXPECT_EQ(line, 1);
    EXPECT_EQ(col, 9);

    EXPECT_TRUE(str_to_filepos("2", &line, &col));
    EXPECT_EQ(line, 2);
    EXPECT_EQ(col, 1);

    EXPECT_TRUE(str_to_filepos("3,1", &line, &col));
    EXPECT_EQ(line, 3);
    EXPECT_EQ(col, 1);

    EXPECT_TRUE(str_to_xfilepos("4", &line, &col));
    EXPECT_EQ(line, 4);
    EXPECT_EQ(col, 0);

    EXPECT_TRUE(str_to_xfilepos("5,1", &line, &col));
    EXPECT_EQ(line, 5);
    EXPECT_EQ(col, 1);

    line = 1111;
    col = 2222;
    EXPECT_FALSE(str_to_filepos("0", &line, &col));
    EXPECT_FALSE(str_to_filepos("1,0", &line, &col));
    EXPECT_FALSE(str_to_filepos("0,1", &line, &col));
    EXPECT_FALSE(str_to_filepos("1,2,3", &line, &col));
    EXPECT_FALSE(str_to_filepos("1,2.3", &line, &col));
    EXPECT_FALSE(str_to_filepos("5,", &line, &col));
    EXPECT_FALSE(str_to_filepos(",5", &line, &col));
    EXPECT_FALSE(str_to_filepos("6.7", &line, &col));
    EXPECT_FALSE(str_to_filepos("2 3", &line, &col));
    EXPECT_FALSE(str_to_filepos("9 ", &line, &col));
    EXPECT_FALSE(str_to_filepos("", &line, &col));
    EXPECT_FALSE(str_to_filepos("\t", &line, &col));
    EXPECT_FALSE(str_to_filepos("44,9x", &line, &col));
    EXPECT_EQ(line, 1111);
    EXPECT_EQ(col, 2222);
}

static void test_buf_umax_to_hex_str(TestContext *ctx)
{
    char buf[HEX_STR_MAX(uintmax_t)];
    memset(buf, '@', sizeof(buf));

    size_t ndigits = buf_umax_to_hex_str(0x98EA412F0ull, buf, 0);
    EXPECT_EQ(ndigits, 9);
    EXPECT_STREQ(buf, "98EA412F0");

    ndigits = buf_umax_to_hex_str(0xE, buf, 0);
    EXPECT_EQ(ndigits, 1);
    EXPECT_STREQ(buf, "E");

    ndigits = buf_umax_to_hex_str(0xF, buf, 4);
    EXPECT_EQ(ndigits, 4);
    EXPECT_STREQ(buf, "000F");

    ndigits = buf_umax_to_hex_str(0, buf, 10);
    EXPECT_EQ(ndigits, 10);
    EXPECT_STREQ(buf, "0000000000");

    ndigits = buf_umax_to_hex_str(0xEF1300, buf, 8);
    EXPECT_EQ(ndigits, 8);
    EXPECT_STREQ(buf, "00EF1300");

    ndigits = buf_umax_to_hex_str(0x1000, buf, 3);
    EXPECT_EQ(ndigits, 4);
    EXPECT_STREQ(buf, "1000");

    ndigits = buf_umax_to_hex_str(0, buf, 0);
    EXPECT_EQ(ndigits, 1);
    EXPECT_STREQ(buf, "0");

    ndigits = buf_umax_to_hex_str(1, buf, 0);
    EXPECT_EQ(ndigits, 1);
    EXPECT_STREQ(buf, "1");

    ndigits = buf_umax_to_hex_str(0x10000000ULL, buf, 0);
    EXPECT_EQ(ndigits, 8);
    EXPECT_STREQ(buf, "10000000");

    ndigits = buf_umax_to_hex_str(0x11111111ULL, buf, 0);
    EXPECT_EQ(ndigits, 8);
    EXPECT_STREQ(buf, "11111111");

    ndigits = buf_umax_to_hex_str(0x123456789ABCDEF0ULL, buf, 0);
    EXPECT_EQ(ndigits, 16);
    EXPECT_STREQ(buf, "123456789ABCDEF0");

    ndigits = buf_umax_to_hex_str(0xFFFFFFFFFFFFFFFFULL, buf, 0);
    EXPECT_EQ(ndigits, 16);
    EXPECT_STREQ(buf, "FFFFFFFFFFFFFFFF");
}

static void test_parse_filesize(TestContext *ctx)
{
    EXPECT_EQ(parse_filesize("0"), 0);
    EXPECT_EQ(parse_filesize("1"), 1);
    EXPECT_EQ(parse_filesize("1KiB"), 1024);
    EXPECT_EQ(parse_filesize("4GiB"), 4LL << 30);
    EXPECT_EQ(parse_filesize("4096MiB"), 4LL << 30);
    EXPECT_EQ(parse_filesize("1234567890"), 1234567890LL);
    EXPECT_EQ(parse_filesize("9GiB"), 9LL << 30);
    EXPECT_EQ(parse_filesize("1GiB"), 1LL << 30);
    EXPECT_EQ(parse_filesize("0GiB"), 0);
    EXPECT_EQ(parse_filesize("0KiB"), 0);
    EXPECT_EQ(parse_filesize("1MiB"), 1LL << 20);
    EXPECT_EQ(parse_filesize("1TiB"), 1LL << 40);
    EXPECT_EQ(parse_filesize("1PiB"), 1LL << 50);
    EXPECT_EQ(parse_filesize("1EiB"), 1LL << 60);

    EXPECT_EQ(parse_filesize("4i"), -EINVAL);
    EXPECT_EQ(parse_filesize("4B"), -EINVAL);
    EXPECT_EQ(parse_filesize("4GB"), -EINVAL);
    EXPECT_EQ(parse_filesize("G4"), -EINVAL);
    EXPECT_EQ(parse_filesize("4G_"), -EINVAL);
    EXPECT_EQ(parse_filesize(" 4G"), -EINVAL);
    EXPECT_EQ(parse_filesize("4G "), -EINVAL);
    EXPECT_EQ(parse_filesize("1K"), -EINVAL);
    EXPECT_EQ(parse_filesize("4G"), -EINVAL);
    EXPECT_EQ(parse_filesize("4096M"), -EINVAL);
    EXPECT_EQ(parse_filesize("9Gi"), -EINVAL);
    EXPECT_EQ(parse_filesize("0K"), -EINVAL);

    char buf[DECIMAL_STR_MAX(uintmax_t) + 4];
    xsnprintf(buf, sizeof buf, "%jd", INTMAX_MAX);
    EXPECT_EQ(parse_filesize(buf), INTMAX_MAX);
    xsnprintf(buf, sizeof buf, "%jdKiB", INTMAX_MAX);
    EXPECT_EQ(parse_filesize(buf), -EOVERFLOW);
    xsnprintf(buf, sizeof buf, "%jdEiB", INTMAX_MAX);
    EXPECT_EQ(parse_filesize(buf), -EOVERFLOW);
    xsnprintf(buf, sizeof buf, "%ju", UINTMAX_MAX);
    EXPECT_EQ(parse_filesize(buf), -EOVERFLOW);
}

static void test_umax_to_str(TestContext *ctx)
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

static void test_uint_to_str(TestContext *ctx)
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

    // See also: test_posix_sanity()
    EXPECT_STREQ(uint_to_str(4294967295u), "4294967295");
}

static void test_ulong_to_str(TestContext *ctx)
{
    unsigned long x = ULONG_MAX;
    char ref[DECIMAL_STR_MAX(x)];
    xsnprintf(ref, sizeof ref, "%lu", x);
    EXPECT_STREQ(ulong_to_str(x), ref);
    EXPECT_STREQ(ulong_to_str(x + 1), "0");
}

static void test_buf_umax_to_str(TestContext *ctx)
{
    char buf[DECIMAL_STR_MAX(uintmax_t)];
    EXPECT_EQ(buf_umax_to_str(0, buf), 1);
    EXPECT_STREQ(buf, "0");
    EXPECT_EQ(buf_umax_to_str(1, buf), 1);
    EXPECT_STREQ(buf, "1");
    EXPECT_EQ(buf_umax_to_str(9, buf), 1);
    EXPECT_STREQ(buf, "9");
    EXPECT_EQ(buf_umax_to_str(10, buf), 2);
    EXPECT_STREQ(buf, "10");
    EXPECT_EQ(buf_umax_to_str(1234567890ull, buf), 10);
    EXPECT_STREQ(buf, "1234567890");
    EXPECT_EQ(buf_umax_to_str(9087654321ull, buf), 10);
    EXPECT_STREQ(buf, "9087654321");
    static_assert(sizeof(buf) > 20);
    EXPECT_EQ(buf_umax_to_str(18446744073709551615ull, buf), 20);
    EXPECT_STREQ(buf, "18446744073709551615");
}

static void test_buf_uint_to_str(TestContext *ctx)
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

static void test_buf_u8_to_str(TestContext *ctx)
{
    // Note: EXPECT_STREQ() only works here if the tests are done in ascending
    // order, since buf_u8_to_str() doesn't null-terminate the buffer
    char buf[4] = {0};
    EXPECT_EQ(buf_u8_to_str(0, buf), 1);
    EXPECT_STREQ(buf, "0");
    EXPECT_EQ(buf_u8_to_str(1, buf), 1);
    EXPECT_STREQ(buf, "1");
    EXPECT_EQ(buf_u8_to_str(9, buf), 1);
    EXPECT_STREQ(buf, "9");
    EXPECT_EQ(buf_u8_to_str(99, buf), 2);
    EXPECT_STREQ(buf, "99");
    EXPECT_EQ(buf_u8_to_str(100, buf), 3);
    EXPECT_STREQ(buf, "100");
    EXPECT_EQ(buf_u8_to_str(255, buf), 3);
    EXPECT_STREQ(buf, "255");
}

static void test_file_permissions_to_str(TestContext *ctx)
{
    char buf[12];
    EXPECT_STREQ("---------", file_permissions_to_str(0, buf));
    EXPECT_STREQ("--------x", file_permissions_to_str(01, buf));
    EXPECT_STREQ("--x--x--x", file_permissions_to_str(0111, buf));
    EXPECT_STREQ("rwx------", file_permissions_to_str(0700, buf));
    EXPECT_STREQ("r--r--r--", file_permissions_to_str(0444, buf));
    EXPECT_STREQ("rw-rw-rw-", file_permissions_to_str(0666, buf));
    EXPECT_STREQ("rwxrwxrwx", file_permissions_to_str(0777, buf));

    EXPECT_STREQ("-----S---", file_permissions_to_str(02000, buf));
    EXPECT_STREQ("-----s---", file_permissions_to_str(02010, buf));
    EXPECT_STREQ("--S------", file_permissions_to_str(04000, buf));
    EXPECT_STREQ("--s------", file_permissions_to_str(04100, buf));
    EXPECT_STREQ("--S--S---", file_permissions_to_str(06000, buf));
    EXPECT_STREQ("--s--s---", file_permissions_to_str(06110, buf));
    EXPECT_STREQ("--s--S---", file_permissions_to_str(06100, buf));
    EXPECT_STREQ("--S--s---", file_permissions_to_str(06010, buf));

#ifdef S_ISVTX
    EXPECT_STREQ("--S--S--T", file_permissions_to_str(07000, buf));
    EXPECT_STREQ("--s--s--t", file_permissions_to_str(07111, buf));
    EXPECT_STREQ("rwsrwsrwt", file_permissions_to_str(07777, buf));
    EXPECT_STREQ("rwSrwSrwT", file_permissions_to_str(07666, buf));
    EXPECT_STREQ("------rwt", file_permissions_to_str(01007, buf));
#else
    EXPECT_STREQ("--S--S---", file_permissions_to_str(07000, buf));
    EXPECT_STREQ("--s--s--x", file_permissions_to_str(07111, buf));
    EXPECT_STREQ("rwsrwsrwx", file_permissions_to_str(07777, buf));
    EXPECT_STREQ("rwSrwSrw-", file_permissions_to_str(07666, buf));
    EXPECT_STREQ("------rwx", file_permissions_to_str(01007, buf));
#endif
}

static void test_human_readable_size(TestContext *ctx)
{
    char buf[HRSIZE_MAX];
    EXPECT_STREQ(human_readable_size(0, buf), "0");
    EXPECT_STREQ(human_readable_size(1, buf), "1");
    EXPECT_STREQ(human_readable_size(10, buf), "10");
    EXPECT_STREQ(human_readable_size(1u << 10, buf), "1 KiB");
    EXPECT_STREQ(human_readable_size(4u << 10, buf), "4 KiB");
    EXPECT_STREQ(human_readable_size(9u << 20, buf), "9 MiB");
    EXPECT_STREQ(human_readable_size(1024u << 10, buf), "1 MiB");
    EXPECT_STREQ(human_readable_size(1024u << 20, buf), "1 GiB");
    EXPECT_STREQ(human_readable_size(1023u << 10, buf), "1023 KiB");
    EXPECT_STREQ(human_readable_size(1023u << 20, buf), "1023 MiB");
    EXPECT_STREQ(human_readable_size(900ull << 30, buf), "900 GiB");
    EXPECT_STREQ(human_readable_size(1ull << 62, buf), "4 EiB");
    EXPECT_STREQ(human_readable_size((1ull << 62) + 232ull, buf), "4 EiB");
    EXPECT_STREQ(human_readable_size(3ull << 61, buf), "6 EiB");
    EXPECT_STREQ(human_readable_size(11ull << 59, buf), "5.50 EiB");

    // Compare to e.g.: numfmt --to=iec --format=%0.7f 7427273
    EXPECT_STREQ(human_readable_size(990, buf), "990");
    EXPECT_STREQ(human_readable_size(1023, buf), "1023");
    EXPECT_STREQ(human_readable_size(1025, buf), "1 KiB");
    EXPECT_STREQ(human_readable_size(1123, buf), "1.09 KiB");
    EXPECT_STREQ(human_readable_size(1124, buf), "1.10 KiB");
    EXPECT_STREQ(human_readable_size(1127, buf), "1.10 KiB");
    EXPECT_STREQ(human_readable_size(4106, buf), "4.01 KiB");
    EXPECT_STREQ(human_readable_size(4192, buf), "4.09 KiB");
    EXPECT_STREQ(human_readable_size(4195, buf), "4.09 KiB");
    EXPECT_STREQ(human_readable_size(4196, buf), "4.10 KiB");
    EXPECT_STREQ(human_readable_size(4197, buf), "4.10 KiB");
    EXPECT_STREQ(human_readable_size(6947713, buf), "6.62 MiB");
    EXPECT_STREQ(human_readable_size(7427273, buf), "7.08 MiB");
    EXPECT_STREQ(human_readable_size(1116691500ull, buf), "1.04 GiB");
    EXPECT_STREQ(human_readable_size(8951980327583ull, buf), "8.14 TiB");
    EXPECT_STREQ(human_readable_size(8951998035275183ull, buf), "7.95 PiB");

    // Some of these results are arguably off by 0.01, but that's fine
    // given the approximate nature of the function and its use cases.
    // These tests are here mostly to exercise edge cases and provide
    // useful feedback when tweaking the algorithm.
    EXPECT_STREQ(human_readable_size(5242803, buf), "4.99 MiB");
    EXPECT_STREQ(human_readable_size(5242804, buf), "5 MiB");
    EXPECT_STREQ(human_readable_size(5242879, buf), "5 MiB");
    EXPECT_STREQ(human_readable_size(5242880, buf), "5 MiB");
    EXPECT_STREQ(human_readable_size(5242881, buf), "5 MiB");

    // Compare with e.g. `units '0xFF00000000000000 bytes' EiB`
    EXPECT_STREQ(human_readable_size(0xFFFFFFFFFFFFFFFFull, buf), "16 EiB");
    EXPECT_STREQ(human_readable_size(0x7FFFFFFFFFFFFFFFull, buf), "8 EiB");
    EXPECT_STREQ(human_readable_size(0x3FFFFFFFFFFFFFFFull, buf), "4 EiB");
    EXPECT_STREQ(human_readable_size(0xFF00000000000000ull, buf), "15.93 EiB");

    const uintmax_t u64pow2max = 0x8000000000000000ull;
    EXPECT_STREQ(human_readable_size(u64pow2max, buf), "8 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 1), buf), "12 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 2), buf), "10 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 3), buf), "9 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 4), buf), "8.50 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 5), buf), "8.25 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 6), buf), "8.12 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 7), buf), "8.06 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 8), buf), "8.03 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 9), buf), "8.01 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 10), buf), "8 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max | (u64pow2max >> 11), buf), "8 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max >> 1, buf), "4 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max >> 2, buf), "2 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max >> 3, buf), "1 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max >> 4, buf), "512 PiB");
    EXPECT_STREQ(human_readable_size(u64pow2max >> 1 | (u64pow2max >> 2), buf), "6 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max >> 1 | (u64pow2max >> 3), buf), "5 EiB");
    EXPECT_STREQ(human_readable_size(u64pow2max >> 1 | (u64pow2max >> 4), buf), "4.50 EiB");
}

static void test_filesize_to_str(TestContext *ctx)
{
    char buf[FILESIZE_STR_MAX];
    EXPECT_STREQ(filesize_to_str(0, buf), "0");
    EXPECT_STREQ(filesize_to_str(1023, buf), "1023");
    EXPECT_STREQ(filesize_to_str(1024, buf), "1 KiB (1024)");

    static_assert(18446744073709551615ull == 0xFFFFFFFFFFFFFFFFull);
    EXPECT_STREQ(filesize_to_str(18446744073709551615ull, buf), "16 EiB (18446744073709551615)");
    EXPECT_STREQ(filesize_to_str(17446744073709551615ull, buf), "15.13 EiB (17446744073709551615)");
}

static void test_filesize_to_str_precise(TestContext *ctx)
{
    char buf[PRECISE_FILESIZE_STR_MAX];
    EXPECT_STREQ(filesize_to_str_precise(0, buf), "0");
    EXPECT_STREQ(filesize_to_str_precise(1, buf), "1");
    EXPECT_STREQ(filesize_to_str_precise(99, buf), "99");
    EXPECT_STREQ(filesize_to_str_precise(1023, buf), "1023");
    EXPECT_STREQ(filesize_to_str_precise(1024, buf), "1KiB");
    EXPECT_STREQ(filesize_to_str_precise(1025, buf), "1025");
    EXPECT_STREQ(filesize_to_str_precise(2047, buf), "2047");
    EXPECT_STREQ(filesize_to_str_precise(2048, buf), "2KiB");
    EXPECT_STREQ(filesize_to_str_precise(3 << 9, buf), "1536"); // Exactly 1.5 KiB
    EXPECT_STREQ(filesize_to_str_precise(3 << 19, buf), "1536KiB"); // Exactly 1.5 MiB
    EXPECT_STREQ(filesize_to_str_precise(3 << 29, buf), "1536MiB"); // Exactly 1.5 GiB
    EXPECT_STREQ(filesize_to_str_precise((3 << 29) + 1, buf), "1610612737");
    EXPECT_STREQ(filesize_to_str_precise(0x8000000000000000ull, buf), "8EiB");
    EXPECT_STREQ(filesize_to_str_precise(0xF000000000000000ull, buf), "15EiB");
    EXPECT_STREQ(filesize_to_str_precise(0xFF00000000000000ull, buf), "16320PiB");
    EXPECT_STREQ(filesize_to_str_precise(0xFFFFFFFFFFFFFFFFull, buf), "18446744073709551615");
}

static void test_u_char_size(TestContext *ctx)
{
    EXPECT_EQ(u_char_size('\0'), 1);
    EXPECT_EQ(u_char_size(' '), 1);
    EXPECT_EQ(u_char_size('z'), 1);
    EXPECT_EQ(u_char_size(0x7E), 1);
    EXPECT_EQ(u_char_size(0x7F), 1);
    EXPECT_EQ(u_char_size(0x80), 2);
    EXPECT_EQ(u_char_size(0x81), 2);
    EXPECT_EQ(u_char_size(0xFF), 2);
    EXPECT_EQ(u_char_size(0x7FE), 2);
    EXPECT_EQ(u_char_size(0x7FF), 2);
    EXPECT_EQ(u_char_size(0x800), 3);
    EXPECT_EQ(u_char_size(0x801), 3);
    EXPECT_EQ(u_char_size(0x1234), 3);
    EXPECT_EQ(u_char_size(0xFFFE), 3);
    EXPECT_EQ(u_char_size(0xFFFF), 3);
    EXPECT_EQ(u_char_size(0x10000), 4);
    EXPECT_EQ(u_char_size(0x10001), 4);
    EXPECT_EQ(u_char_size(0x10FFFE), 4);
    EXPECT_EQ(u_char_size(0x10FFFF), 4);
    EXPECT_EQ(u_char_size(0x110000), 1);
    EXPECT_EQ(u_char_size(0x110001), 1);
    EXPECT_EQ(u_char_size(UINT32_MAX), 1);
}

static void test_u_char_width(TestContext *ctx)
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
    EXPECT_EQ(u_char_width('\t'), 2);
    EXPECT_EQ(u_char_width('\n'), 2);
    EXPECT_EQ(u_char_width('\r'), 2);
    EXPECT_EQ(u_char_width(0x1F), 2);
    EXPECT_EQ(u_char_width(0x7F), 2);

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

static void test_u_to_lower(TestContext *ctx)
{
    EXPECT_EQ(u_to_lower('A'), 'a');
    EXPECT_EQ(u_to_lower('Z'), 'z');
    EXPECT_EQ(u_to_lower('a'), 'a');
    EXPECT_EQ(u_to_lower('0'), '0');
    EXPECT_EQ(u_to_lower('~'), '~');
    EXPECT_EQ(u_to_lower('@'), '@');
    EXPECT_EQ(u_to_lower('\0'), '\0');
}

static void test_u_to_upper(TestContext *ctx)
{
    EXPECT_EQ(u_to_upper('a'), 'A');
    EXPECT_EQ(u_to_upper('z'), 'Z');
    EXPECT_EQ(u_to_upper('A'), 'A');
    EXPECT_EQ(u_to_upper('0'), '0');
    EXPECT_EQ(u_to_upper('~'), '~');
    EXPECT_EQ(u_to_upper('@'), '@');
    EXPECT_EQ(u_to_upper('\0'), '\0');
}

static void test_u_is_lower(TestContext *ctx)
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

static void test_u_is_upper(TestContext *ctx)
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

static void test_u_is_ascii_upper(TestContext *ctx)
{
    EXPECT_TRUE(u_is_ascii_upper('A'));
    EXPECT_TRUE(u_is_ascii_upper('Z'));
    EXPECT_FALSE(u_is_ascii_upper('a'));
    EXPECT_FALSE(u_is_ascii_upper('z'));
    EXPECT_FALSE(u_is_ascii_upper('0'));
    EXPECT_FALSE(u_is_ascii_upper('9'));
    EXPECT_FALSE(u_is_ascii_upper('@'));
    EXPECT_FALSE(u_is_ascii_upper('['));
    EXPECT_FALSE(u_is_ascii_upper('{'));
    EXPECT_FALSE(u_is_ascii_upper('\0'));
    EXPECT_FALSE(u_is_ascii_upper('\t'));
    EXPECT_FALSE(u_is_ascii_upper(' '));
    EXPECT_FALSE(u_is_ascii_upper(0x7F));
    EXPECT_FALSE(u_is_ascii_upper(0x1D440));
    EXPECT_FALSE(u_is_ascii_upper(UNICODE_MAX_VALID_CODEPOINT));
}

static void test_u_is_cntrl(TestContext *ctx)
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

static void test_u_is_unicode(TestContext *ctx)
{
    EXPECT_TRUE(u_is_unicode(0));
    EXPECT_TRUE(u_is_unicode(1));
    EXPECT_TRUE(u_is_unicode(UNICODE_MAX_VALID_CODEPOINT));
    EXPECT_FALSE(u_is_unicode(UNICODE_MAX_VALID_CODEPOINT + 1));
}

static void test_u_is_zero_width(TestContext *ctx)
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

static void test_u_is_special_whitespace(TestContext *ctx)
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

static void test_u_is_unprintable(TestContext *ctx)
{
    // Private-use characters ------------------------------------------------
    // • https://www.unicode.org/faq/private_use.html#pua2
    // • https://www.unicode.org/versions/latest/core-spec/chapter-2/#G286941:~:text=Private%2Duse,-Usage

    // There are three ranges of private-use characters in the standard.
    // The main range in the BMP is U+E000..U+F8FF, containing 6,400
    // private-use characters.
    EXPECT_FALSE(u_is_unprintable(0xE000));
    EXPECT_FALSE(u_is_unprintable(0xF8FF));

    // ... there are also two large ranges of supplementary private-use
    // characters, consisting of most of the code points on planes 15
    // and 16: U+F0000..U+FFFFD and U+100000..U+10FFFD. Together those
    // ranges allocate another 131,068 private-use characters.
    EXPECT_FALSE(u_is_unprintable(0xF0000));
    EXPECT_FALSE(u_is_unprintable(0xFFFFD));
    EXPECT_FALSE(u_is_unprintable(0x100000));
    EXPECT_FALSE(u_is_unprintable(0x10FFFD));

    // Surrogates ------------------------------------------------------------
    // • https://www.unicode.org/versions/latest/core-spec/chapter-3/#G2630
    // • https://www.unicode.org/versions/latest/core-spec/chapter-2/#G286941:~:text=Surrogate,-Permanently
    EXPECT_TRUE(u_is_unprintable(0xD800));
    EXPECT_TRUE(u_is_unprintable(0xDBFF));
    EXPECT_TRUE(u_is_unprintable(0xDC00));
    EXPECT_TRUE(u_is_unprintable(0xDFFF));

    // Non-characters --------------------------------------------------------
    // • https://www.unicode.org/faq/private_use.html#noncharacters
    // • https://www.unicode.org/versions/latest/core-spec/chapter-2/#G286941:~:text=Noncharacter,-Permanently
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

static void test_u_str_width(TestContext *ctx)
{
    static const char ustr[] =
        "\xE0\xB8\x81\xE0\xB8\xB3\xE0\xB9\x81\xE0\xB8\x9E\xE0\xB8"
        "\x87\xE0\xB8\xA1\xE0\xB8\xB5\xE0\xB8\xAB\xE0\xB8\xB9"
    ;

    EXPECT_EQ(u_str_width("foo"), 3);
    EXPECT_EQ(u_str_width(ustr), 7);
}

static void test_u_set_char_raw(TestContext *ctx)
{
    unsigned char buf[UTF8_MAX_SEQ_LEN] = "";
    EXPECT_EQ(sizeof(buf), 4);
    EXPECT_EQ(u_set_char_raw(buf, 'a'), 1);
    EXPECT_EQ(buf[0], 'a');

    EXPECT_EQ(u_set_char_raw(buf, '\0'), 1);
    EXPECT_EQ(buf[0], '\0');

    EXPECT_EQ(u_set_char_raw(buf, 0x1F), 1);
    EXPECT_EQ(buf[0], 0x1F);

    EXPECT_EQ(u_set_char_raw(buf, 0x7F), 1);
    EXPECT_EQ(buf[0], 0x7F);

    EXPECT_EQ(u_set_char_raw(buf, 0x7FF), 2);
    EXPECT_EQ(buf[0], 0xDF);
    EXPECT_EQ(buf[1], 0xBF);

    EXPECT_EQ(u_set_char_raw(buf, 0xFF45), 3);
    EXPECT_EQ(buf[0], 0xEF);
    EXPECT_EQ(buf[1], 0xBD);
    EXPECT_EQ(buf[2], 0x85);

    EXPECT_EQ(u_set_char_raw(buf, 0x1F311), 4);
    EXPECT_EQ(buf[0], 0xF0);
    EXPECT_EQ(buf[1], 0x9F);
    EXPECT_EQ(buf[2], 0x8C);
    EXPECT_EQ(buf[3], 0x91);

    buf[1] = 0x88;
    EXPECT_EQ(u_set_char_raw(buf, 0x110000), 1);
    EXPECT_EQ(buf[0], 0);
    EXPECT_EQ(buf[1], 0x88);

    EXPECT_EQ(u_set_char_raw(buf, 0x110042), 1);
    EXPECT_EQ(buf[0], 0x42);
    EXPECT_EQ(buf[1], 0x88);
}

static void test_u_set_char(TestContext *ctx)
{
    unsigned char buf[U_SET_CHAR_MAXLEN] = "";
    EXPECT_EQ(sizeof(buf), 4);
    EXPECT_EQ(u_set_char(buf, 'a'), 1);
    EXPECT_EQ(buf[0], 'a');

    EXPECT_EQ(u_set_char(buf, 0x00DF), 2);
    EXPECT_EQ(buf[0], 0xC3);
    EXPECT_EQ(buf[1], 0x9F);

    EXPECT_EQ(u_set_char(buf, 0x0E01), 3);
    EXPECT_EQ(buf[0], 0xE0);
    EXPECT_EQ(buf[1], 0xB8);
    EXPECT_EQ(buf[2], 0x81);

    EXPECT_EQ(UTF8_MAX_SEQ_LEN, 4);
    EXPECT_EQ(u_set_char(buf, 0x1F914), 4);
    EXPECT_EQ(buf[0], 0xF0);
    EXPECT_EQ(buf[1], 0x9F);
    EXPECT_EQ(buf[2], 0xA4);
    EXPECT_EQ(buf[3], 0x94);

    EXPECT_EQ(U_SET_HEX_LEN, 4);
    EXPECT_EQ(u_set_char(buf, 0x10FFFF), 4);
    EXPECT_EQ(buf[0], '<');
    EXPECT_EQ(buf[1], '?');
    EXPECT_EQ(buf[2], '?');
    EXPECT_EQ(buf[3], '>');

    EXPECT_EQ(u_set_char(buf, '\0'), 2);
    EXPECT_EQ(buf[0], '^');
    EXPECT_EQ(buf[1], '@');

    EXPECT_EQ(u_set_char(buf, '\t'), 2);
    EXPECT_EQ(buf[0], '^');
    EXPECT_EQ(buf[1], 'I');

    EXPECT_EQ(u_set_char(buf, 0x1F), 2);
    EXPECT_EQ(buf[0], '^');
    EXPECT_EQ(buf[1], '_');

    EXPECT_EQ(u_set_char(buf, 0x7F), 2);
    EXPECT_EQ(buf[0], '^');
    EXPECT_EQ(buf[1], '?');

    EXPECT_EQ(u_set_char(buf, 0x80), 4);
    EXPECT_EQ(buf[0], '<');
    EXPECT_EQ(buf[1], '?');
    EXPECT_EQ(buf[2], '?');
    EXPECT_EQ(buf[3], '>');

    EXPECT_EQ(u_set_char(buf, 0x7E), 1);
    EXPECT_EQ(buf[0], '~');

    EXPECT_EQ(u_set_char(buf, 0x20), 1);
    EXPECT_EQ(buf[0], ' ');

    unsigned char c = 0xA5;
    EXPECT_EQ(u_set_char(buf, 0x110000 + c), 4);
    EXPECT_EQ(buf[0], '<');
    EXPECT_EQ(buf[1], '5');
    EXPECT_EQ(buf[2], 'b');
    EXPECT_EQ(buf[3], '>');
    c = -c;
    EXPECT_UINT_EQ(c, 0x5b);
}

static void test_u_make_printable(TestContext *ctx)
{
    char buf[5];
    MakePrintableFlags flags = 0;
    EXPECT_EQ(sizeof(buf), U_SET_CHAR_MAXLEN + 1);
    memset(buf, '_', sizeof(buf));
    EXPECT_EQ(u_make_printable(STRN("\xffxyz"), buf, sizeof(buf), flags), 4);
    EXPECT_STREQ(buf, "<ff>");

    // Enough space for `U_SET_CHAR_MAXLEN + 1` is needed, regardless of
    // the CodePoint encountered, so 5 is the minimum remaining space
    // required in order to write more than just a null-terminator
    memset(buf, '_', sizeof(buf));
    EXPECT_TRUE(sizeof(buf) >= 5);
    EXPECT_EQ(u_make_printable(STRN("12345"), buf, 1, flags), 0);
    EXPECT_EQ(u_make_printable(STRN("12345"), buf, 2, flags), 0);
    EXPECT_EQ(u_make_printable(STRN("12345"), buf, 3, flags), 0);
    EXPECT_EQ(u_make_printable(STRN("12345"), buf, 4, flags), 0);
    EXPECT_EQ(buf[0], '\0');
    EXPECT_EQ(buf[1], '_');
    EXPECT_EQ(buf[2], '_');
    EXPECT_EQ(buf[3], '_');
    EXPECT_EQ(buf[4], '_');
    memset(buf, '_', sizeof(buf));
    EXPECT_EQ(u_make_printable(STRN("12345"), buf, 5, flags), 1);
    EXPECT_EQ(buf[0], '1');
    EXPECT_EQ(buf[1], '\0');
    EXPECT_EQ(buf[2], '_');
    EXPECT_EQ(buf[3], '_');
    EXPECT_EQ(buf[4], '_');

    memset(buf, '_', sizeof(buf));
    EXPECT_EQ(u_make_printable(STRN("\x7F\n123"), buf, 5, flags), 2);
    EXPECT_STREQ(buf, "^?");
    EXPECT_EQ(buf[3], '_');

    flags |= MPF_C0_SYMBOLS;
    memset(buf, '_', sizeof(buf));
    EXPECT_EQ(u_make_printable(STRN("\x7F\n123"), buf, 5, flags), 3);
    EXPECT_STREQ(buf, "\xE2\x90\xA1"); // UTF-8 encoding of U+2421
    EXPECT_EQ(buf[4], '_');

    memset(buf, '_', sizeof(buf));
    EXPECT_EQ(u_make_printable(STRN("\0xyz"), buf, 5, flags), 3);
    EXPECT_STREQ(buf, "\xE2\x90\x80"); // UTF-8 encoding of U+2400
    EXPECT_EQ(buf[4], '_');
}

static void test_u_get_char(TestContext *ctx)
{
    static const char a[] = "//";
    size_t idx = 0;
    EXPECT_UINT_EQ(u_get_char(a, sizeof a, &idx), '/');
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(a, sizeof a, &idx), '/');
    ASSERT_EQ(idx, 2);
    EXPECT_UINT_EQ(u_get_char(a, sizeof a, &idx), 0);
    ASSERT_EQ(idx, 3);

    // Non-character U+FDD0.
    // • https://www.unicode.org/faq/private_use.html#nonchar4:~:text=EF%20B7%2090
    static const char nc1[] = "\xEF\xB7\x90";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(nc1, sizeof nc1, &idx), 0xFDD0);
    ASSERT_EQ(idx, 3);
    EXPECT_TRUE(u_is_unprintable(0xFDD0));

    // Non-character U+FFFE
    // • https://www.unicode.org/faq/private_use.html#nonchar4:~:text=EF%20BF%20B%23
    static const char nc2[] = "\xEF\xBF\xBE";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(nc2, sizeof nc2, &idx), 0xFFFE);
    ASSERT_EQ(idx, 3);
    EXPECT_TRUE(u_is_unprintable(0xFFFE));

    // -----------------------------------------------------------------------

    // "In UTF-8, the code point sequence <004D, 0430, 4E8C, 10302> is
    // represented as <4D D0 B0 E4 BA 8C F0 90 8C 82>, where <4D>
    // corresponds to U+004D, <D0 B0> corresponds to U+0430, <E4 BA 8C>
    // corresponds to U+4E8C, and <F0 90 8C 82> corresponds to U+10302."
    // - https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G31703
    static const char b[] = "\x4D\xD0\xB0\xE4\xBA\x8C\xF0\x90\x8C\x82";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(b, sizeof b, &idx), 0x004D);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(b, sizeof b, &idx), 0x0430);
    ASSERT_EQ(idx, 3);
    EXPECT_UINT_EQ(u_get_char(b, sizeof b, &idx), 0x4E8C);
    ASSERT_EQ(idx, 6);
    EXPECT_UINT_EQ(u_get_char(b, sizeof b, &idx), 0x10302);
    ASSERT_EQ(idx, 10);

    // "The byte sequence <F4 80 83 92> is well-formed, because every
    // byte in that sequence matches a byte range in a row of the table
    // (the last row)."
    // - https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G27288
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(STRN("\xF4\x80\x83\x92"), &idx), 0x1000D2);
    EXPECT_EQ(idx, 4);

    // Overlong encodings are consumed as individual bytes and
    // each byte returned negated (to indicate that it's invalid).
    // See also: u_get_nonascii(), u_seq_len_ok(), u_char_size()

    // Overlong (2 byte) encoding of U+002F ('/').
    // "The byte sequence <C0 AF> is ill-formed, because C0 is not
    // well-formed in the “First Byte” column."
    static const char ol1[] = "\xC0\xAF";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(ol1, sizeof ol1, &idx), -0xC0u);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(ol1, sizeof ol1, &idx), -0xAFu);
    ASSERT_EQ(idx, 2);

    // Overlong (3 byte) encoding of U+002F ('/')
    static const char ol2[] = "\xE0\x80\xAF";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(ol2, sizeof ol2, &idx), -0xE0u);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(ol2, sizeof ol2, &idx), -0x80u);
    ASSERT_EQ(idx, 2);
    EXPECT_UINT_EQ(u_get_char(ol2, sizeof ol2, &idx), -0xAFu);
    ASSERT_EQ(idx, 3);

    // Overlong (2 byte) encoding of U+0041 ('A')
    static const char ol3[] = "\xC1\x81";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(ol3, sizeof ol3, &idx), -0xC1u);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(ol3, sizeof ol3, &idx), -0x81u);
    ASSERT_EQ(idx, 2);

    // "The byte sequence <E0 9F 80> is ill-formed, because in the row
    // where E0 is well-formed as a first byte, 9F is not well-formed
    // as a second byte."
    static const char ol4[] = "\xE0\x9F\x80";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(ol4, sizeof ol4, &idx), -0xE0u);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(ol4, sizeof ol4, &idx), -0x9Fu);
    ASSERT_EQ(idx, 2);
    EXPECT_UINT_EQ(u_get_char(ol4, sizeof ol4, &idx), -0x80u);
    ASSERT_EQ(idx, 3);

    // "For example, in processing the UTF-8 code unit sequence
    // <F0 80 80 41>, the only formal requirement mandated by Unicode
    // conformance for a converter is that the <41> be processed and
    // correctly interpreted as <U+0041>. The converter could return
    // <U+FFFD, U+0041>, handling <F0 80 80> as a single error, or
    // <U+FFFD, U+FFFD, U+FFFD, U+0041>, handling each byte of
    // <F0 80 80> as a separate error, or could take other approaches
    // to signalling <F0 80 80> as an ill-formed code unit subsequence."
    // - https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G48534
    static const char e1[] = "\xF0\x80\x80\x41";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(e1, sizeof e1, &idx), -0xF0u);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(e1, sizeof e1, &idx), -0x80u);
    ASSERT_EQ(idx, 2);
    EXPECT_UINT_EQ(u_get_char(e1, sizeof e1, &idx), -0x80u);
    ASSERT_EQ(idx, 3);
    EXPECT_UINT_EQ(u_get_char(e1, sizeof e1, &idx), 0x41u);
    ASSERT_EQ(idx, 4);

    // The following test cases are taken from Unicode's examples in
    // Tables 3-{8,9,10,11}. We return the negation of each individual
    // byte in an ill-formed sequence, instead of the "maximal subpart"
    // approach mentioned there. This is explicitly permitted by the
    // spec and is more flexible for our purposes:
    //
    // "Although the Unicode Standard does not require this practice for
    // conformance, the following text describes this practice and gives
    // detailed examples."
    //
    // - https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G66453

    // Table 3-8. … Non-Shortest Form Sequences
    // https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G67519
    static const char s8[] = "\xC0\xAF\xE0\x80\xBF\xF0\x81\x82\x41";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(s8, sizeof s8, &idx), -0xC0u);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(s8, sizeof s8, &idx), -0xAFu);
    ASSERT_EQ(idx, 2);
    EXPECT_UINT_EQ(u_get_char(s8, sizeof s8, &idx), -0xE0u);
    ASSERT_EQ(idx, 3);
    EXPECT_UINT_EQ(u_get_char(s8, sizeof s8, &idx), -0x80u);
    ASSERT_EQ(idx, 4);
    EXPECT_UINT_EQ(u_get_char(s8, sizeof s8, &idx), -0xBFu);
    ASSERT_EQ(idx, 5);
    EXPECT_UINT_EQ(u_get_char(s8, sizeof s8, &idx), -0xF0u);
    ASSERT_EQ(idx, 6);
    EXPECT_UINT_EQ(u_get_char(s8, sizeof s8, &idx), -0x81u);
    ASSERT_EQ(idx, 7);
    EXPECT_UINT_EQ(u_get_char(s8, sizeof s8, &idx), -0x82u);
    ASSERT_EQ(idx, 8);
    EXPECT_UINT_EQ(u_get_char(s8, sizeof s8, &idx), 0x41u);
    ASSERT_EQ(idx, 9);

    // Table 3-9. … Ill-Formed Sequences for Surrogates
    // https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G67520
    static const char s9[] = "\xED\xA0\x80\xED\xBF\xBF\xED\xAF\x41";
    EXPECT_TRUE(u_is_surrogate(0xD800)); // "\xED\xA0\x80"
    EXPECT_TRUE(u_is_unprintable(0xD800));
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(s9, sizeof s9, &idx), -0xEDu);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(s9, sizeof s9, &idx), -0xA0u);
    ASSERT_EQ(idx, 2);
    EXPECT_UINT_EQ(u_get_char(s9, sizeof s9, &idx), -0x80u);
    ASSERT_EQ(idx, 3);
    EXPECT_UINT_EQ(u_get_char(s9, sizeof s9, &idx), -0xEDu);
    ASSERT_EQ(idx, 4);
    EXPECT_UINT_EQ(u_get_char(s9, sizeof s9, &idx), -0xBFu);
    ASSERT_EQ(idx, 5);
    EXPECT_UINT_EQ(u_get_char(s9, sizeof s9, &idx), -0xBFu);
    ASSERT_EQ(idx, 6);
    EXPECT_UINT_EQ(u_get_char(s9, sizeof s9, &idx), -0xEDu);
    ASSERT_EQ(idx, 7);
    EXPECT_UINT_EQ(u_get_char(s9, sizeof s9, &idx), -0xAFu);
    ASSERT_EQ(idx, 8);
    EXPECT_UINT_EQ(u_get_char(s9, sizeof s9, &idx), 0x41u);
    ASSERT_EQ(idx, 9);

    // Table 3-10. … Other Ill-Formed Sequences
    // https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G68064
    static const char s10[] = "\xF4\x91\x92\x93\xFF\x41\x80\xBF\x42";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(s10, sizeof s10, &idx), -0xF4u);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(s10, sizeof s10, &idx), -0x91u);
    ASSERT_EQ(idx, 2);
    EXPECT_UINT_EQ(u_get_char(s10, sizeof s10, &idx), -0x92u);
    ASSERT_EQ(idx, 3);
    EXPECT_UINT_EQ(u_get_char(s10, sizeof s10, &idx), -0x93u);
    ASSERT_EQ(idx, 4);
    EXPECT_UINT_EQ(u_get_char(s10, sizeof s10, &idx), -0xFFu);
    ASSERT_EQ(idx, 5);
    EXPECT_UINT_EQ(u_get_char(s10, sizeof s10, &idx), 0x41u);
    ASSERT_EQ(idx, 6);
    EXPECT_UINT_EQ(u_get_char(s10, sizeof s10, &idx), -0x80u);
    ASSERT_EQ(idx, 7);
    EXPECT_UINT_EQ(u_get_char(s10, sizeof s10, &idx), -0xBFu);
    ASSERT_EQ(idx, 8);
    EXPECT_UINT_EQ(u_get_char(s10, sizeof s10, &idx), 0x42u);
    ASSERT_EQ(idx, 9);

    // Table 3-11. … Truncated Sequences
    // https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G68202
    static const char s11[] = "\xE1\x80\xE2\xF0\x91\x92\xF1\xBF\x41";
    idx = 0;
    EXPECT_UINT_EQ(u_get_char(s11, sizeof s11, &idx), -0xE1u);
    ASSERT_EQ(idx, 1);
    EXPECT_UINT_EQ(u_get_char(s11, sizeof s11, &idx), -0x80u);
    ASSERT_EQ(idx, 2);
    EXPECT_UINT_EQ(u_get_char(s11, sizeof s11, &idx), -0xE2u);
    ASSERT_EQ(idx, 3);
    EXPECT_UINT_EQ(u_get_char(s11, sizeof s11, &idx), -0xF0u);
    ASSERT_EQ(idx, 4);
    EXPECT_UINT_EQ(u_get_char(s11, sizeof s11, &idx), -0x91u);
    ASSERT_EQ(idx, 5);
    EXPECT_UINT_EQ(u_get_char(s11, sizeof s11, &idx), -0x92u);
    ASSERT_EQ(idx, 6);
    EXPECT_UINT_EQ(u_get_char(s11, sizeof s11, &idx), -0xF1u);
    ASSERT_EQ(idx, 7);
    EXPECT_UINT_EQ(u_get_char(s11, sizeof s11, &idx), -0xBFu);
    ASSERT_EQ(idx, 8);
    EXPECT_UINT_EQ(u_get_char(s11, sizeof s11, &idx), 0x41u);
    ASSERT_EQ(idx, 9);

    // TODO: Provide explicit coverage for each row of Table 3-7:
    // https://www.unicode.org/versions/Unicode16.0.0/core-spec/chapter-3/#G27506
}

static void test_u_prev_char(TestContext *ctx)
{
    const unsigned char *buf = "\xE6\xB7\xB1\xE5\x9C\xB3\xE5\xB8\x82"; // 深圳市
    size_t idx = 9;
    CodePoint c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(c, 0x5E02);
    EXPECT_EQ(idx, 6);
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(c, 0x5733);
    EXPECT_EQ(idx, 3);
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(c, 0x6DF1);
    EXPECT_EQ(idx, 0);

    idx = 1;
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(c, -0xE6u);
    EXPECT_EQ(idx, 0);

    buf = "Shenzhen";
    idx = 8;
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(c, 'n');
    EXPECT_EQ(idx, 7);
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(c, 'e');
    EXPECT_EQ(idx, 6);

    buf = "\xF0\x9F\xA5\xA3\xF0\x9F\xA5\xA4"; // 🥣🥤
    idx = 8;
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(c, 0x1F964);
    EXPECT_EQ(idx, 4);
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(c, 0x1F963);
    EXPECT_EQ(idx, 0);

    buf = "\xF0\xF5";
    idx = 2;
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(-c, buf[1]);
    EXPECT_EQ(idx, 1);
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(-c, buf[0]);
    EXPECT_EQ(idx, 0);

    buf = "\xF5\xF0";
    idx = 2;
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(-c, buf[1]);
    EXPECT_EQ(idx, 1);
    c = u_prev_char(buf, &idx);
    EXPECT_UINT_EQ(-c, buf[0]);
    EXPECT_EQ(idx, 0);
}

static void test_u_skip_chars(TestContext *ctx)
{
    EXPECT_EQ(u_skip_chars("12345", 5), 5);
    EXPECT_EQ(u_skip_chars("12345", 3), 3);
    EXPECT_EQ(u_skip_chars("12345", 0), 0);

    // Display: <c1>  x  ^G  y  😁  z  ^G  .
    static const char str[] = "\xc1x\ay\xF0\x9F\x98\x81z\a.";
    const size_t w = u_str_width(str);
    const size_t n = sizeof(str) - 1;
    EXPECT_EQ(w, 4 + 1 + 2 + 1 + 2 + 1 + 2 + 1);
    EXPECT_EQ(w, 14);
    EXPECT_EQ(n, 11);

    EXPECT_EQ(u_skip_chars(str, 1), 1); // <c1>
    EXPECT_EQ(u_skip_chars(str, 2), 1);
    EXPECT_EQ(u_skip_chars(str, 3), 1);
    EXPECT_EQ(u_skip_chars(str, 4), 1);
    EXPECT_EQ(u_skip_chars(str, 5), 2); // x
    EXPECT_EQ(u_skip_chars(str, 6), 3); // ^G
    EXPECT_EQ(u_skip_chars(str, 7), 3);
    EXPECT_EQ(u_skip_chars(str, 8), 4); // y
    EXPECT_EQ(u_skip_chars(str, 9), 8); // 😁
    EXPECT_EQ(u_skip_chars(str, 10), 8);
    EXPECT_EQ(u_skip_chars(str, 11), 9); // z
    EXPECT_EQ(u_skip_chars(str, 12), 10); // ^G
    EXPECT_EQ(u_skip_chars(str, 13), 10);
    EXPECT_EQ(14, w);
    EXPECT_EQ(u_skip_chars(str, 14), n); // .
    EXPECT_TRUE(15 > w);
    EXPECT_EQ(u_skip_chars(str, 15), n);
    EXPECT_EQ(u_skip_chars(str, 16), n);
}

static void test_ptr_array(TestContext *ctx)
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

    ptr_array_init(&a, 0);
    EXPECT_EQ(a.alloc, 0);
    EXPECT_NULL(a.ptrs);
    ptr_array_free(&a);
    ptr_array_init(&a, 1);
    EXPECT_EQ(a.alloc, 8);
    EXPECT_NONNULL(a.ptrs);
    ptr_array_free(&a);

    // ptr_array_trim_nulls() should remove everything (i.e. not leave 1
    // trailing NULL) when all elements are NULL
    ptr_array_init(&a, 0);
    ptr_array_append(&a, NULL);
    ptr_array_append(&a, NULL);
    ptr_array_append(&a, NULL);
    EXPECT_EQ(a.count, 3);
    ptr_array_trim_nulls(&a);
    EXPECT_EQ(a.count, 0);
    ptr_array_free(&a);
}

static void test_ptr_array_move(TestContext *ctx)
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

    ptr_array_move(&a, 3, 3);
    EXPECT_STREQ(a.ptrs[3], "A");
    ptr_array_move(&a, 1, 1);
    EXPECT_STREQ(a.ptrs[1], "B");
    ptr_array_move(&a, 0, 0);
    EXPECT_STREQ(a.ptrs[0], "F");

    ptr_array_free(&a);
}

static void test_ptr_array_insert(TestContext *ctx)
{
    PointerArray a = PTR_ARRAY_INIT;
    ptr_array_insert(&a, xstrdup("D"), 0);
    ptr_array_insert(&a, xstrdup("C"), 0);
    ptr_array_insert(&a, xstrdup("B"), 0);
    ptr_array_insert(&a, xstrdup("A"), 0);
    EXPECT_EQ(a.count, 4);

    ptr_array_insert(&a, xstrdup("X"), 1);
    ptr_array_insert(&a, xstrdup("Y"), 3);
    EXPECT_EQ(a.count, 6);

    ptr_array_insert(&a, xstrdup("Z"), a.count);
    EXPECT_EQ(a.count, 7);

    EXPECT_STREQ(a.ptrs[0], "A");
    EXPECT_STREQ(a.ptrs[1], "X");
    EXPECT_STREQ(a.ptrs[2], "B");
    EXPECT_STREQ(a.ptrs[3], "Y");
    EXPECT_STREQ(a.ptrs[4], "C");
    EXPECT_STREQ(a.ptrs[5], "D");
    EXPECT_STREQ(a.ptrs[6], "Z");

    ptr_array_free(&a);
}

static void test_list(TestContext *ctx)
{
    ListHead a, b, c;
    list_init(&b);
    EXPECT_TRUE(list_empty(&b));

    list_insert_before(&a, &b);
    EXPECT_FALSE(list_empty(&a));
    EXPECT_FALSE(list_empty(&b));
    EXPECT_PTREQ(a.next, &b);
    EXPECT_PTREQ(a.prev, &b);
    EXPECT_PTREQ(b.next, &a);
    EXPECT_PTREQ(b.prev, &a);

    list_insert_after(&c, &b);
    EXPECT_FALSE(list_empty(&a));
    EXPECT_FALSE(list_empty(&b));
    EXPECT_FALSE(list_empty(&c));
    EXPECT_PTREQ(a.next, &b);
    EXPECT_PTREQ(a.prev, &c);
    EXPECT_PTREQ(b.next, &c);
    EXPECT_PTREQ(b.prev, &a);
    EXPECT_PTREQ(c.next, &a);
    EXPECT_PTREQ(c.prev, &b);

    list_remove(&b);
    EXPECT_FALSE(list_empty(&a));
    EXPECT_FALSE(list_empty(&c));
    EXPECT_PTREQ(a.next, &c);
    EXPECT_PTREQ(a.prev, &c);
    EXPECT_PTREQ(c.next, &a);
    EXPECT_PTREQ(c.prev, &a);
    EXPECT_NULL(b.next);
    EXPECT_NULL(b.prev);
}

static void test_hashmap(TestContext *ctx)
{
    static const char strings[][8] = {
        "foo", "bar", "quux", "etc", "",
        "A", "B", "C", "D", "E", "F", "G",
        "a", "b", "c", "d", "e", "f", "g",
        "test", "1234567", "..", "...",
        "\x01\x02\x03 \t\xfe\xff",
    };

    HashMap map;
    hashmap_init(&map, ARRAYLEN(strings), HMAP_NO_FLAGS);
    ASSERT_NONNULL(map.entries);
    EXPECT_EQ(map.mask, 31);
    EXPECT_EQ(map.count, 0);
    EXPECT_NULL(hashmap_find(&map, "foo"));

    static const char value[] = "VALUE";
    FOR_EACH_I(i, strings) {
        const char *key = strings[i];
        ASSERT_EQ(key[sizeof(strings[0]) - 1], '\0');
        EXPECT_PTREQ(hashmap_insert(&map, xstrdup(key), (void*)value), value);
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
        EXPECT_STREQ(e->key, NULL);
        EXPECT_UINT_EQ(e->hash, 0xdead);
        EXPECT_NULL(hashmap_find(&map, key));
    }

    EXPECT_EQ(map.count, 0);
    it = hashmap_iter(&map);
    EXPECT_FALSE(hashmap_next(&it));

    EXPECT_PTREQ(hashmap_insert(&map, xstrdup("new"), (void*)value), value);
    ASSERT_NONNULL(hashmap_find(&map, "new"));
    EXPECT_STREQ(hashmap_find(&map, "new")->key, "new");
    EXPECT_EQ(map.count, 1);

    FOR_EACH_I(i, strings) {
        const char *key = strings[i];
        EXPECT_PTREQ(hashmap_insert(&map, xstrdup(key), (void*)value), value);
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
        EXPECT_STREQ(e->key, NULL);
        EXPECT_UINT_EQ(e->hash, 0xdead);
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

    hashmap_init(&map, 0, HMAP_NO_FLAGS);
    ASSERT_NONNULL(map.entries);
    EXPECT_EQ(map.mask, 7);
    EXPECT_EQ(map.count, 0);
    hashmap_free(&map, NULL);
    EXPECT_NULL(map.entries);

    hashmap_init(&map, 13, HMAP_NO_FLAGS);
    ASSERT_NONNULL(map.entries);
    EXPECT_EQ(map.mask, 31);
    EXPECT_EQ(map.count, 0);

    for (unsigned int i = 1; i <= 380; i++) {
        char key[4];
        ASSERT_TRUE(buf_uint_to_str(i, key) < sizeof(key));
        EXPECT_PTREQ(hashmap_insert(&map, xstrdup(key), (void*)value), value);
        HashMapEntry *e = hashmap_find(&map, key);
        ASSERT_NONNULL(e);
        EXPECT_STREQ(e->key, key);
        EXPECT_PTREQ(e->value, value);
    }

    EXPECT_EQ(map.count, 380);
    EXPECT_EQ(map.mask, 511);
    hashmap_free(&map, NULL);

    static const char val[] = "VAL";
    char *key = xstrdup("KEY");
    EXPECT_NULL(hashmap_insert_or_replace(&map, key, (char*)val));
    EXPECT_EQ(map.count, 1);
    EXPECT_STREQ(hashmap_get(&map, "KEY"), val);

    static const char new_val[] = "NEW";
    char *duplicate_key = xstrdup(key);
    EXPECT_PTREQ(val, hashmap_insert_or_replace(&map, duplicate_key, (char*)new_val));
    EXPECT_EQ(map.count, 1);
    EXPECT_STREQ(hashmap_get(&map, "KEY"), new_val);
    hashmap_free(&map, NULL);
}

static void test_hashset(TestContext *ctx)
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
    hashset_init(&set, ARRAYLEN(strings), false);
    EXPECT_EQ(set.nr_entries, 0);
    EXPECT_EQ(set.table_size, 16);
    EXPECT_EQ(set.grow_at, 12);
    EXPECT_NONNULL(set.table);
    EXPECT_NONNULL(set.hash);
    EXPECT_NONNULL(set.equal);
    EXPECT_NULL(hashset_get(&set, "foo", 3));

    HashSetIter iter = hashset_iter(&set);
    EXPECT_PTREQ(iter.set, &set);
    EXPECT_NULL(iter.entry);
    EXPECT_EQ(iter.idx, 0);
    EXPECT_FALSE(hashset_next(&iter));
    EXPECT_PTREQ(iter.set, &set);
    EXPECT_NULL(iter.entry);
    EXPECT_EQ(iter.idx, 0);

    FOR_EACH_I(i, strings) {
        hashset_insert(&set, strings[i], strlen(strings[i]));
    }

    FOR_EACH_I(i, strings) {
        EXPECT_TRUE(hashset_next(&iter));
    }
    EXPECT_FALSE(hashset_next(&iter));
    EXPECT_FALSE(hashset_next(&iter));

    EXPECT_EQ(set.nr_entries, ARRAYLEN(strings));
    EXPECT_NONNULL(hashset_get(&set, "\t\xff\x80\b", 4));
    EXPECT_NONNULL(hashset_get(&set, "foo", 3));
    EXPECT_NONNULL(hashset_get(&set, "Foo", 3));

    EXPECT_NULL(hashset_get(&set, "FOO", 3));
    EXPECT_NULL(hashset_get(&set, "", 0));
    EXPECT_NULL(hashset_get(&set, NULL, 0));
    EXPECT_NULL(hashset_get(&set, "\0", 1));

    const char *last_string = strings[ARRAYLEN(strings) - 1];
    EXPECT_NONNULL(hashset_get(&set, last_string, strlen(last_string)));

    FOR_EACH_I(i, strings) {
        const char *str = strings[i];
        const size_t len = strlen(str);
        EXPECT_NONNULL(hashset_get(&set, str, len));
        EXPECT_NULL(hashset_get(&set, str, len - 1));
        EXPECT_NULL(hashset_get(&set, str + 1, len - 1));
    }

    hashset_free(&set);
    hashset_init(&set, 0, true);
    EXPECT_EQ(set.nr_entries, 0);
    hashset_insert(&set, STRN("foo"));
    hashset_insert(&set, STRN("Foo"));
    EXPECT_EQ(set.nr_entries, 1);
    EXPECT_NONNULL(hashset_get(&set, STRN("foo")));
    EXPECT_NONNULL(hashset_get(&set, STRN("FOO")));
    EXPECT_NONNULL(hashset_get(&set, STRN("fOO")));
    hashset_free(&set);

    // Check that hashset_insert() returns existing entries instead of
    // inserting duplicates
    hashset_init(&set, 0, false);
    EXPECT_EQ(set.nr_entries, 0);
    HashSetEntry *e1 = hashset_insert(&set, STRN("foo"));
    EXPECT_EQ(e1->str_len, 3);
    EXPECT_STREQ(e1->str, "foo");
    EXPECT_EQ(set.nr_entries, 1);
    HashSetEntry *e2 = hashset_insert(&set, STRN("foo"));
    EXPECT_PTREQ(e1, e2);
    EXPECT_EQ(set.nr_entries, 1);
    hashset_free(&set);

    hashset_init(&set, 0, false);
    // Initial table size should be 16 (minimum + load factor + rounding)
    EXPECT_EQ(set.table_size, 16);
    for (unsigned int i = 1; i <= 80; i++) {
        char buf[4];
        size_t len = buf_uint_to_str(i, buf);
        ASSERT_TRUE(len < sizeof(buf));
        hashset_insert(&set, buf, len);
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

static void test_intmap(TestContext *ctx)
{
    IntMap map = INTMAP_INIT;
    EXPECT_NULL(intmap_find(&map, 0));
    EXPECT_NULL(intmap_get(&map, 0));
    intmap_free(&map, free);

    static const char value[] = "value";
    EXPECT_NULL(intmap_insert_or_replace(&map, 0, xstrdup(value)));
    EXPECT_NULL(intmap_insert_or_replace(&map, 1, xstrdup(value)));
    EXPECT_NULL(intmap_insert_or_replace(&map, 2, xstrdup(value)));
    EXPECT_NULL(intmap_insert_or_replace(&map, 4, xstrdup(value)));
    EXPECT_EQ(map.count, 4);
    EXPECT_EQ(map.mask, 7);

    char *replaced = intmap_insert_or_replace(&map, 0, xstrdup(value));
    EXPECT_STREQ(replaced, value);
    free(replaced);
    EXPECT_EQ(map.tombstones, 0);
    EXPECT_STREQ(intmap_get(&map, 0), value);

    char *removed = intmap_remove(&map, 0);
    EXPECT_STREQ(removed, value);
    free(removed);
    EXPECT_EQ(map.tombstones, 1);
    EXPECT_NULL(intmap_get(&map, 0));

    EXPECT_NULL(intmap_insert_or_replace(&map, 100, xstrdup(value)));
    EXPECT_NULL(intmap_insert_or_replace(&map, 488, xstrdup(value)));
    EXPECT_NULL(intmap_insert_or_replace(&map, 899, xstrdup(value)));
    EXPECT_NULL(intmap_insert_or_replace(&map, 256, xstrdup(value)));
    EXPECT_EQ(map.count, 7);
    EXPECT_EQ(map.mask, 15);

    intmap_free(&map, free);
}

static void test_next_multiple(TestContext *ctx)
{
    EXPECT_EQ(next_multiple(0, 1), 0);
    EXPECT_EQ(next_multiple(1, 1), 1);
    EXPECT_EQ(next_multiple(2, 1), 2);
    EXPECT_EQ(next_multiple(3, 1), 3);
    EXPECT_EQ(next_multiple(0, 2), 0);
    EXPECT_EQ(next_multiple(1, 2), 2);
    EXPECT_EQ(next_multiple(5, 2), 6);
    EXPECT_EQ(next_multiple(1, 8), 8);
    EXPECT_EQ(next_multiple(3, 8), 8);
    EXPECT_EQ(next_multiple(8, 8), 8);
    EXPECT_EQ(next_multiple(9, 8), 16);
    EXPECT_EQ(next_multiple(0, 8), 0);
    EXPECT_EQ(next_multiple(0, 16), 0);
    EXPECT_EQ(next_multiple(1, 16), 16);
    EXPECT_EQ(next_multiple(123, 16), 128);
    EXPECT_EQ(next_multiple(4, 64), 64);
    EXPECT_EQ(next_multiple(80, 64), 128);
    EXPECT_EQ(next_multiple(256, 256), 256);
    EXPECT_EQ(next_multiple(257, 256), 512);
    EXPECT_EQ(next_multiple(8000, 256), 8192);

    const size_t size_max = (size_t)-1;
    const size_t pow2_max = size_max & ~(size_max >> 1);
    EXPECT_TRUE(IS_POWER_OF_2(pow2_max));
    EXPECT_UINT_EQ(next_multiple(size_max, 1), size_max);
    EXPECT_UINT_EQ(next_multiple(pow2_max, 1), pow2_max);
    EXPECT_UINT_EQ(next_multiple(pow2_max, pow2_max), pow2_max);

    // Note: returns 0 on overflow
    EXPECT_UINT_EQ(next_multiple(pow2_max + 1, pow2_max), 0);
    EXPECT_UINT_EQ(next_multiple(size_max + 0, pow2_max), 0);
    EXPECT_UINT_EQ(next_multiple(size_max - 1, pow2_max), 0);

    const size_t a = pow2_max >> 1;
    EXPECT_UINT_EQ(next_multiple(a, a), a);
    EXPECT_UINT_EQ(next_multiple(a + 1, a), pow2_max);
}

static void test_next_pow2(TestContext *ctx)
{
    EXPECT_UINT_EQ(next_pow2(0), 1);
    EXPECT_UINT_EQ(next_pow2(1), 1);
    EXPECT_UINT_EQ(next_pow2(2), 2);
    EXPECT_UINT_EQ(next_pow2(3), 4);
    EXPECT_UINT_EQ(next_pow2(4), 4);
    EXPECT_UINT_EQ(next_pow2(5), 8);
    EXPECT_UINT_EQ(next_pow2(8), 8);
    EXPECT_UINT_EQ(next_pow2(9), 16);
    EXPECT_UINT_EQ(next_pow2(17), 32);
    EXPECT_UINT_EQ(next_pow2(61), 64);
    EXPECT_UINT_EQ(next_pow2(64), 64);
    EXPECT_UINT_EQ(next_pow2(65), 128);
    EXPECT_UINT_EQ(next_pow2(200), 256);
    EXPECT_UINT_EQ(next_pow2(1000), 1024);
    EXPECT_UINT_EQ(next_pow2(5500), 8192);

    const size_t size_max = (size_t)-1;
    const size_t pow2_max = ~(size_max >> 1);
    EXPECT_TRUE(IS_POWER_OF_2(pow2_max));
    EXPECT_UINT_EQ(next_pow2(size_max >> 1), pow2_max);
    EXPECT_UINT_EQ(next_pow2(pow2_max), pow2_max);
    EXPECT_UINT_EQ(next_pow2(pow2_max - 1), pow2_max);

    // Note: returns 0 on overflow
    EXPECT_UINT_EQ(next_pow2(pow2_max + 1), 0);
    EXPECT_UINT_EQ(next_pow2(size_max), 0);
    EXPECT_UINT_EQ(next_pow2(size_max - 1), 0);
}

static void test_popcount(TestContext *ctx)
{
    EXPECT_EQ(u32_popcount(0), 0);
    EXPECT_EQ(u32_popcount(1), 1);
    EXPECT_EQ(u32_popcount(11), 3);
    EXPECT_EQ(u32_popcount(128), 1);
    EXPECT_EQ(u32_popcount(255), 8);
    EXPECT_EQ(u32_popcount(UINT32_MAX), 32);
    EXPECT_EQ(u32_popcount(UINT32_MAX - 1), 31);
    EXPECT_EQ(u32_popcount(U32(0xE10F02C9)), 13);

    EXPECT_EQ(u64_popcount(0), 0);
    EXPECT_EQ(u64_popcount(1), 1);
    EXPECT_EQ(u64_popcount(255), 8);
    EXPECT_EQ(u64_popcount(UINT64_MAX), 64);
    EXPECT_EQ(u64_popcount(UINT64_MAX - 1), 63);
    EXPECT_EQ(u64_popcount(U64(0xFFFFFFFFFF)), 40);
    EXPECT_EQ(u64_popcount(U64(0x10000000000)), 1);
    EXPECT_EQ(u64_popcount(U64(0x9010F0EEC2003B70)), 24);

    for (unsigned int i = 0; i < 32; i++) {
        IEXPECT_EQ(u32_popcount(UINT32_MAX << i), 32 - i);
        IEXPECT_EQ(u32_popcount(UINT32_MAX >> i), 32 - i);
        IEXPECT_EQ(u32_popcount(U32(1) << i), 1);
    }

    for (unsigned int i = 0; i < 64; i++) {
        IEXPECT_EQ(u64_popcount(UINT64_MAX << i), 64 - i);
        IEXPECT_EQ(u64_popcount(UINT64_MAX >> i), 64 - i);
        IEXPECT_EQ(u64_popcount(U64(1) << i), 1);
    }
}

static void test_ctz(TestContext *ctx)
{
    EXPECT_EQ(u32_ctz(1), 0);
    EXPECT_EQ(u32_ctz(11), 0);
    EXPECT_EQ(u32_ctz(127), 0);
    EXPECT_EQ(u32_ctz(128), 7);
    EXPECT_EQ(u32_ctz(129), 0);
    EXPECT_EQ(u32_ctz(130), 1);
    EXPECT_EQ(u32_ctz(255), 0);
    EXPECT_EQ(u32_ctz(UINT32_MAX), 0);
    EXPECT_EQ(u32_ctz(UINT32_MAX - 1), 1);
    EXPECT_EQ(u32_ctz(0xE10F02C9u), 0);
    EXPECT_EQ(u32_ctz(0xE10F02CCu), 2);

    EXPECT_EQ(umax_ctz(1), 0);
    EXPECT_EQ(umax_ctz(11), 0);
    EXPECT_EQ(umax_ctz(127), 0);
    EXPECT_EQ(umax_ctz(128), 7);
    EXPECT_EQ(umax_ctz(129), 0);
    EXPECT_EQ(umax_ctz(130), 1);
    EXPECT_EQ(umax_ctz(255), 0);
    EXPECT_EQ(umax_ctz(0xE10F02C9u), 0);
    EXPECT_EQ(umax_ctz(0xE10F02CCu), 2);
    EXPECT_EQ(umax_ctz(0x8000000000000000ull), 63);
    EXPECT_EQ(umax_ctz(UINTMAX_MAX), 0);
    EXPECT_EQ(umax_ctz(UINTMAX_MAX - 1), 1);
    EXPECT_EQ(umax_ctz(UINTMAX_MAX - 7), 3);
}

static void test_ffs(TestContext *ctx)
{
    EXPECT_EQ(u32_ffs(0), 0);
    EXPECT_EQ(u32_ffs(1), 1);
    EXPECT_EQ(u32_ffs(6), 2);
    EXPECT_EQ(u32_ffs(8), 4);
    EXPECT_EQ(u32_ffs(255), 1);
    EXPECT_EQ(u32_ffs(256), 9);
    EXPECT_EQ(u32_ffs(~U32(255)), 9);
    EXPECT_EQ(u32_ffs(UINT32_MAX), 1);
    EXPECT_EQ(u32_ffs(UINT32_MAX - 1), 2);
    EXPECT_EQ(u32_ffs(U32(1) << 31), 32);
    EXPECT_EQ(u32_ffs(U32(1) << 30), 31);
    EXPECT_EQ(u32_ffs(U32(1) << 18), 19);
}

static void test_lsbit(TestContext *ctx)
{
    EXPECT_EQ(u32_lsbit(0), 0);
    EXPECT_EQ(u32_lsbit(1), 1);
    EXPECT_EQ(u32_lsbit(2), 2);
    EXPECT_EQ(u32_lsbit(3), 1);
    EXPECT_EQ(u32_lsbit(4), 4);
    EXPECT_EQ(u32_lsbit(255), 1);
    EXPECT_EQ(u32_lsbit(256), 256);
    EXPECT_EQ(u32_lsbit(257), 1);
    EXPECT_EQ(u32_lsbit(258), 2);
    EXPECT_EQ(u32_lsbit(1u << 31), 1u << 31);
    EXPECT_EQ(u32_lsbit(1u << 30), 1u << 30);
    EXPECT_EQ(u32_lsbit(7u << 30), 1u << 30);
    EXPECT_EQ(u32_lsbit(UINT32_MAX), 1);
    EXPECT_EQ(u32_lsbit(UINT32_MAX << 25), 1u << 25);

    for (uint32_t i = 1; i < 70; i++) {
        uint32_t lsb = u32_lsbit(i);
        IEXPECT_TRUE(IS_POWER_OF_2(lsb));
        IEXPECT_TRUE(lsb & i);
    }
}

static void test_msbit(TestContext *ctx)
{
    EXPECT_EQ(size_msbit(0), 0);
    EXPECT_EQ(size_msbit(1), 1);
    EXPECT_EQ(size_msbit(2), 2);
    EXPECT_EQ(size_msbit(3), 2);
    EXPECT_EQ(size_msbit(4), 4);
    EXPECT_EQ(size_msbit(7), 4);
    EXPECT_EQ(size_msbit(8), 8);
    EXPECT_EQ(size_msbit(255), 128);
    EXPECT_EQ(size_msbit(256), 256);
    EXPECT_UINT_EQ(size_msbit(0x1FFFu), 0x1000u);
    EXPECT_UINT_EQ(size_msbit(0xFFFFu), 0x8000u);

    const size_t max = SIZE_MAX;
    const size_t max_pow2 = ~(max >> 1);
    EXPECT_UINT_EQ(size_msbit(max), max_pow2);
    EXPECT_UINT_EQ(size_msbit(max - 1), max_pow2);
    EXPECT_UINT_EQ(size_msbit(max_pow2), max_pow2);
    EXPECT_UINT_EQ(size_msbit(max_pow2 - 1), max_pow2 >> 1);
    EXPECT_UINT_EQ(size_msbit(max_pow2 + 1), max_pow2);

    for (size_t i = 1; i < 70; i++) {
        size_t msb = size_msbit(i);
        IEXPECT_TRUE(IS_POWER_OF_2(msb));
        IEXPECT_TRUE(msb & i);
    }
}

static void test_clz(TestContext *ctx)
{
    EXPECT_EQ(u64_clz(1), 63);
    EXPECT_EQ(u64_clz(2), 62);
    EXPECT_EQ(u64_clz(3), 62);
    EXPECT_EQ(u64_clz(1ULL << 10), 53);
    EXPECT_EQ(u64_clz(1ULL << 55), 8);
    EXPECT_EQ(u64_clz(1ULL << 63), 0);
    EXPECT_EQ(u64_clz(UINT64_MAX), 0);
    EXPECT_EQ(u64_clz(UINT64_MAX >> 1), 1);
    EXPECT_EQ(u64_clz(UINT64_MAX >> 3), 3);
}

static void test_umax_bitwidth(TestContext *ctx)
{
    EXPECT_EQ(umax_bitwidth(0), 0);
    EXPECT_EQ(umax_bitwidth(1), 1);
    EXPECT_EQ(umax_bitwidth(2), 2);
    EXPECT_EQ(umax_bitwidth(3), 2);
    EXPECT_EQ(umax_bitwidth(0x80), 8);
    EXPECT_EQ(umax_bitwidth(0xFF), 8);
    EXPECT_EQ(umax_bitwidth(0x10081), 17);
    EXPECT_EQ(umax_bitwidth(1ULL << 62), 63);
    EXPECT_EQ(umax_bitwidth(0xFFFFFFFFFFFFFFFFULL), 64);
}

static void test_umax_count_base16_digits(TestContext *ctx)
{
    EXPECT_EQ(umax_count_base16_digits(0x0), 1);
    EXPECT_EQ(umax_count_base16_digits(0x1), 1);
    EXPECT_EQ(umax_count_base16_digits(0x2), 1);
    EXPECT_EQ(umax_count_base16_digits(0x3), 1);
    EXPECT_EQ(umax_count_base16_digits(0x4), 1);
    EXPECT_EQ(umax_count_base16_digits(0x5), 1);
    EXPECT_EQ(umax_count_base16_digits(0x6), 1);
    EXPECT_EQ(umax_count_base16_digits(0x7), 1);
    EXPECT_EQ(umax_count_base16_digits(0x8), 1);
    EXPECT_EQ(umax_count_base16_digits(0x9), 1);
    EXPECT_EQ(umax_count_base16_digits(0xA), 1);
    EXPECT_EQ(umax_count_base16_digits(0xB), 1);
    EXPECT_EQ(umax_count_base16_digits(0xF), 1);
    EXPECT_EQ(umax_count_base16_digits(0x10), 2);
    EXPECT_EQ(umax_count_base16_digits(0x111), 3);
    EXPECT_EQ(umax_count_base16_digits(0xFF11), 4);
    EXPECT_EQ(umax_count_base16_digits(0x80000000ULL), 8);
    EXPECT_EQ(umax_count_base16_digits(0x800000000ULL), 9);
    EXPECT_EQ(umax_count_base16_digits(0X98EA412F0ULL), 9);
    EXPECT_EQ(umax_count_base16_digits(0x8000000000000000ULL), 16);
    EXPECT_EQ(umax_count_base16_digits(0xFFFFFFFFFFFFFFFFULL), 16);
}

static void test_path_dirname_basename(TestContext *ctx)
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

static void test_path_relative(TestContext *ctx)
{
    static const struct {
        const char *cwd;
        const char *path;
        const char *result;
    } tests[] = { // NOTE: at most 2 ".." components allowed in relative name
        { "/", "/", "/" },
        { "/", "/file", "file" },
        { "/a/b/c/d", "/a/b/file", "../../file" },
        { "/a/b/c/d/e", "/a/b/file", "/a/b/file" },
        { "/a/foobar", "/a/foo/file", "../foo/file" },
        { "/home/user", "/home/userx", "../userx"},
        { "/home/user", "/home/use", "../use"},
        { "/home/user", "/home/user", "."},
        { "/home", "/home/user", "user"},
    };
    FOR_EACH_I(i, tests) {
        char *result = path_relative(tests[i].path, tests[i].cwd);
        IEXPECT_STREQ(tests[i].result, result);
        free(result);
    }
}

static void test_path_slice_relative(TestContext *ctx)
{
    static const char abs[] = "/a/b/c/d";
    EXPECT_PTREQ(path_slice_relative(abs, "/a/b/c/d/e"), abs);
    EXPECT_PTREQ(path_slice_relative(abs, "/a/b/file"), abs);
    EXPECT_STREQ(path_slice_relative(abs, "/a/b/c/d"), ".");
    EXPECT_STREQ(path_slice_relative(abs, "/a/b/c"), "d");
    EXPECT_STREQ(path_slice_relative(abs, "/"), "a/b/c/d");
    EXPECT_PTREQ(path_slice_relative(abs, "/a/b/c"), abs + STRLEN("/a/b/c/"));
    EXPECT_STREQ(path_slice_relative("/", "/"), "/");
    EXPECT_STREQ(path_slice_relative("/aa/bb/ccX", "/aa/bb/cc"), "/aa/bb/ccX");
}

static void test_short_filename_cwd(TestContext *ctx)
{
    const StringView home = STRING_VIEW("/home/user");
    char *s = short_filename_cwd("/home/user", "/home/user", &home);
    EXPECT_STREQ(s, ".");
    free(s);

    s = short_filename_cwd("/home/use", "/home/user", &home);
    EXPECT_STREQ(s, "../use");
    free(s);

    s = short_filename_cwd("/a/b/c/d", "/a/x/y/file", &home);
    EXPECT_STREQ(s, "/a/b/c/d");
    free(s);

    s = short_filename_cwd("/home/user/file", "/home/user/cwd", &home);
    EXPECT_STREQ(s, "~/file");
    free(s);

    static const char abs[] = "/a/b";
    static const char cwd[] = "/a/x/c";
    char *rel = path_relative(abs, cwd);
    EXPECT_TRUE(strlen(abs) < strlen(rel));
    EXPECT_STREQ(rel, "../../b");
    free(rel);
    s = short_filename_cwd(abs, cwd, &home);
    EXPECT_STREQ(s, "/a/b");
    free(s);
}

static void test_short_filename(TestContext *ctx)
{
    const StringView home = STRING_VIEW("/home/user");
    static const char rel[] = "test/main.c";
    char *abs = path_absolute(rel);
    ASSERT_NONNULL(abs);
    char *s = short_filename(abs, &home);
    EXPECT_STREQ(s, rel);
    free(abs);
    free(s);

    s = short_filename("/home/user/subdir/file.txt", &home);
    EXPECT_STREQ(s, "~/subdir/file.txt");
    free(s);

    s = short_filename("/x/y/z", &home);
    EXPECT_STREQ(s, "/x/y/z");
    free(s);
}

static void test_path_absolute(TestContext *ctx)
{
    char *path = path_absolute("///dev///");
    EXPECT_STREQ(path, "/dev");
    free(path);

    path = path_absolute("///dev///..///dev//");
    EXPECT_STREQ(path, "/dev");
    free(path);

    path = path_absolute("///dev//n0nexist3nt-file");
    EXPECT_STREQ(path, "/dev/n0nexist3nt-file");
    free(path);

    errno = 0;
    path = path_absolute("/dev/-n0n-existent-dir-[];/file");
    EXPECT_EQ(errno, ENOENT);
    EXPECT_STREQ(path, NULL);
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

    static const char linkpath[] = "./build/test/../test/test-symlink";
    if (symlink("../gen/platform.mk", linkpath) != 0) {
        TEST_FAIL("symlink() failed: %s", strerror(errno));
        return;
    }
    test_pass(ctx);

    path = path_absolute(linkpath);
    EXPECT_EQ(unlink(linkpath), 0);
    ASSERT_NONNULL(path);
    EXPECT_STREQ(path_basename(path), "platform.mk");
    free(path);
}

static void test_path_join(TestContext *ctx)
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

    p = path_join_sv(strview("foo"), strview("bar"), true);
    EXPECT_STREQ(p, "foo/bar/");
    free(p);
    p = path_join_sv(strview("foo"), strview("bar"), false);
    EXPECT_STREQ(p, "foo/bar");
    free(p);
    p = path_join_sv(strview(""), strview(""), true);
    EXPECT_STREQ(p, "");
    free(p);
    p = path_join_sv(strview("/"), strview(""), true);
    EXPECT_STREQ(p, "/");
    free(p);
    p = path_join_sv(strview(""), strview("file"), true);
    EXPECT_STREQ(p, "file/");
    free(p);
    p = path_join_sv(strview(""), strview("file"), false);
    EXPECT_STREQ(p, "file");
    free(p);
    p = path_join_sv(strview(""), strview("file/"), true);
    EXPECT_STREQ(p, "file/");
    free(p);
}

static void test_path_parent(TestContext *ctx)
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

static void test_wrapping_increment(TestContext *ctx)
{
    EXPECT_EQ(wrapping_increment(0, 1), 0);
    EXPECT_EQ(wrapping_increment(3, 5), 4);
    EXPECT_EQ(wrapping_increment(4, 5), 0);
    EXPECT_EQ(wrapping_increment(0, 5), 1);

    for (size_t m = 1; m < 9; m++) {
        for (size_t x = 0; x < m; x++) {
            EXPECT_EQ(wrapping_increment(x, m), (x + 1) % m);
        }
    }
}

static void test_wrapping_decrement(TestContext *ctx)
{
    EXPECT_EQ(wrapping_decrement(0, 1), 0);
    EXPECT_EQ(wrapping_decrement(1, 450), 0);
    EXPECT_EQ(wrapping_decrement(0, 450), 449);
    EXPECT_EQ(wrapping_decrement(449, 450), 448);

    for (size_t m = 1; m < 9; m++) {
        EXPECT_EQ(wrapping_decrement(0, m), m - 1);
        for (size_t x = 1; x < m; x++) {
            EXPECT_EQ(wrapping_decrement(x, m), (x - 1) % m);
        }
    }
}

static void test_saturating_increment(TestContext *ctx)
{
    EXPECT_UINT_EQ(saturating_increment(0, 0), 0);
    EXPECT_UINT_EQ(saturating_increment(1, 1), 1);
    EXPECT_UINT_EQ(saturating_increment(6, 7), 7);
    EXPECT_UINT_EQ(saturating_increment(7, 7), 7);

    const size_t m = SIZE_MAX;
    EXPECT_UINT_EQ(saturating_increment(m - 2, m), m - 1);
    EXPECT_UINT_EQ(saturating_increment(m - 1, m), m);
    EXPECT_UINT_EQ(saturating_increment(m, m), m);
}

static void test_saturating_decrement(TestContext *ctx)
{
    EXPECT_UINT_EQ(saturating_decrement(0), 0);
    EXPECT_UINT_EQ(saturating_decrement(1), 0);
    EXPECT_UINT_EQ(saturating_decrement(2), 1);
    EXPECT_UINT_EQ(saturating_decrement(3), 2);

    const size_t m = SIZE_MAX;
    EXPECT_UINT_EQ(saturating_decrement(m), m - 1);
}

static void test_saturating_subtract(TestContext *ctx)
{
    EXPECT_UINT_EQ(saturating_subtract(0, 1), 0);
    EXPECT_UINT_EQ(saturating_subtract(1, 1), 0);
    EXPECT_UINT_EQ(saturating_subtract(2, 1), 1);
    EXPECT_UINT_EQ(saturating_subtract(191, 170), 21);

    const size_t m = SIZE_MAX;
    EXPECT_UINT_EQ(saturating_subtract(m - 1, m - 2), 1);
    EXPECT_UINT_EQ(saturating_subtract(m - 1, m), 0);
    EXPECT_UINT_EQ(saturating_subtract(m, m), 0);
    EXPECT_UINT_EQ(saturating_subtract(m, m - 1), 1);
    EXPECT_UINT_EQ(saturating_subtract(m, m - 2), 2);
    EXPECT_UINT_EQ(saturating_subtract(0, m), 0);
    EXPECT_UINT_EQ(saturating_subtract(1, m), 0);
    EXPECT_UINT_EQ(saturating_subtract(m, 1), m - 1);
}

static void test_size_multiply_overflows(TestContext *ctx)
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

static void test_size_add_overflows(TestContext *ctx)
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

static void test_xmul(TestContext *ctx)
{
    const size_t halfmax = SIZE_MAX / 2;
    EXPECT_UINT_EQ(xmul(2, halfmax), 2 * halfmax);
    EXPECT_UINT_EQ(xmul(8, 8), 64);
    EXPECT_UINT_EQ(xmul(1, SIZE_MAX), SIZE_MAX);
    EXPECT_UINT_EQ(xmul(2000, 1), 2000);
}

static void test_xadd(TestContext *ctx)
{
    const size_t max = SIZE_MAX;
    EXPECT_UINT_EQ(xadd(max - 1, 1), max);
    EXPECT_UINT_EQ(xadd(8, 8), 16);
    EXPECT_UINT_EQ(xadd(0, 0), 0);

    EXPECT_UINT_EQ(xadd3(max - 3, 2, 1), max);
    EXPECT_UINT_EQ(xadd3(11, 9, 5071), 5091);
    EXPECT_UINT_EQ(xadd3(0, 0, 0), 0);
}

static void test_mem_intern(TestContext *ctx)
{
    const char *ptrs[256];
    char str[8];
    for (unsigned int i = 0; i < ARRAYLEN(ptrs); i++) {
        size_t len = buf_uint_to_str(i, str);
        ptrs[i] = mem_intern(str, len);
    }

    EXPECT_STREQ(ptrs[0], "0");
    EXPECT_STREQ(ptrs[1], "1");
    EXPECT_STREQ(ptrs[101], "101");
    EXPECT_STREQ(ptrs[255], "255");

    for (unsigned int i = 0; i < ARRAYLEN(ptrs); i++) {
        size_t len = buf_uint_to_str(i, str);
        const char *ptr = mem_intern(str, len);
        EXPECT_PTREQ(ptr, ptrs[i]);
    }
}

static void test_read_file(TestContext *ctx)
{
    char *buf = NULL;
    ASSERT_EQ(read_file("/dev/null", &buf, 64), 0);
    ASSERT_NONNULL(buf);
    EXPECT_UINT_EQ((unsigned char)buf[0], '\0');
    free(buf);

    buf = NULL;
    errno = 0;
    EXPECT_EQ(read_file("test/data/", &buf, 64), -1);
    EXPECT_EQ(errno, EISDIR);
    EXPECT_NULL(buf);
    free(buf);

    buf = NULL;
    errno = 0;
    EXPECT_EQ(read_file("test/data/3lines.txt", &buf, 1), -1);
    EXPECT_EQ(errno, EFBIG);
    EXPECT_NULL(buf);
    free(buf);

    ssize_t size = read_file("test/data/3lines.txt", &buf, 512);
    EXPECT_EQ(size, 26);
    ASSERT_NONNULL(buf);
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

static void test_xfopen(TestContext *ctx)
{
    static const char modes[][4] = {"a", "a+", "r", "r+", "w", "w+"};
    FOR_EACH_I(i, modes) {
        FILE *f = xfopen("/dev/null", modes[i], O_CLOEXEC, 0666);
        IEXPECT_TRUE(f && fclose(f) == 0);
    }
}

static void test_xstdio(TestContext *ctx)
{
    FILE *f = xfopen("/dev/null", "r+", O_CLOEXEC, 0666);
    ASSERT_NONNULL(f);

    char buf[16];
    EXPECT_NULL(xfgets(buf, sizeof(buf), f));
    EXPECT_NE(xfputs("str", f), EOF);
    EXPECT_EQ(xfputc(' ', f), ' ');
    EXPECT_EQ(xfprintf(f, "fmt %d", 42), 6);
    EXPECT_EQ(xfflush(f), 0);
    EXPECT_EQ(fclose(f), 0);
}

static void test_fd_set_cloexec(TestContext *ctx)
{
    int fd = xopen("/dev/null", O_RDONLY, 0);
    ASSERT_TRUE(fd >= 0);
    int flags = fcntl(fd, F_GETFD);
    EXPECT_TRUE(flags >= 0);
    EXPECT_EQ(flags & FD_CLOEXEC, 0);

    EXPECT_TRUE(fd_set_cloexec(fd, true));
    flags = fcntl(fd, F_GETFD);
    EXPECT_TRUE(flags > 0);
    EXPECT_EQ(flags & FD_CLOEXEC, FD_CLOEXEC);

    // This set of tests is repeated twice, in order to check the special
    // case where the F_SETFD operation can be omitted because FD_CLOEXEC
    // was already set as requested
    for (size_t i = 0; i < 2; i++) {
        IEXPECT_TRUE(fd_set_cloexec(fd, false));
        flags = fcntl(fd, F_GETFD);
        IEXPECT_TRUE(flags >= 0);
        IEXPECT_EQ(flags & FD_CLOEXEC, 0);
    }

    xclose(fd);
}

static void test_fd_set_nonblock(TestContext *ctx)
{
    int fd = xopen("/dev/null", O_RDONLY, 0);
    ASSERT_TRUE(fd >= 0);
    int flags = fcntl(fd, F_GETFL);
    EXPECT_TRUE(flags >= 0);
    EXPECT_EQ(flags & O_NONBLOCK, 0);

    EXPECT_TRUE(fd_set_nonblock(fd, true));
    flags = fcntl(fd, F_GETFL);
    EXPECT_TRUE(flags > 0);
    EXPECT_EQ(flags & O_NONBLOCK, O_NONBLOCK);

    for (size_t i = 0; i < 2; i++) {
        IEXPECT_TRUE(fd_set_nonblock(fd, false));
        flags = fcntl(fd, F_GETFL);
        IEXPECT_TRUE(flags >= 0);
        IEXPECT_EQ(flags & O_NONBLOCK, 0);
    }

    xclose(fd);
}

static void test_fork_exec(TestContext *ctx)
{
    int fd[3];
    fd[0] = xopen("/dev/null", O_RDWR | O_CLOEXEC, 0);
    ASSERT_TRUE(fd[0] > 0);
    fd[1] = fd[0];
    fd[2] = fd[0];

    const char *argv[] = {"sh", "-c", "exit 95", NULL};
    pid_t pid = fork_exec(argv, fd, 0, 0, true);
    ASSERT_NE(pid, -1);
    int r = wait_child(pid);
    EXPECT_EQ(r, 95);

    argv[0] = "sleep";
    argv[1] = "5";
    argv[2] = NULL;
    pid = fork_exec(argv, fd, 0, 0, true);
    ASSERT_NE(pid, -1);
    EXPECT_EQ(kill(pid, SIGINT), 0);
    r = wait_child(pid);
    EXPECT_TRUE(r >= 256);
    EXPECT_EQ(r >> 8, SIGINT);

    EXPECT_EQ(xclose(fd[0]), 0);
}

static void test_xmemmem(TestContext *ctx)
{
    static const char haystack[] = "finding a needle in a haystack";
    const char *needle = xmemmem(haystack, sizeof(haystack), STRN("needle"));
    ASSERT_NONNULL(needle);
    EXPECT_PTREQ(needle, haystack + 10);

    needle = xmemmem(haystack, sizeof(haystack), "\0", 1);
    ASSERT_NONNULL(needle);
    EXPECT_PTREQ(needle, haystack + sizeof(haystack) - 1);

    needle = xmemmem(haystack, sizeof(haystack) - 1, "\0", 1);
    EXPECT_NULL(needle);

    needle = xmemmem(haystack, sizeof(haystack), STRN("in "));
    ASSERT_NONNULL(needle);
    EXPECT_PTREQ(needle, haystack + 17);

    needle = xmemmem(haystack, sizeof(haystack) - 1, STRN("haystack"));
    ASSERT_NONNULL(needle);
    EXPECT_PTREQ(needle, haystack + 22);

    needle = xmemmem(haystack, sizeof(haystack) - 1, STRN("haystacks"));
    EXPECT_NULL(needle);

    needle = xmemmem(haystack, sizeof(haystack), STRN("haystacks"));
    EXPECT_NULL(needle);
}

static void test_xmemrchr(TestContext *ctx)
{
    static const char str[] = "123456789 abcdefedcba 987654321";
    EXPECT_PTREQ(xmemrchr(str, '9', sizeof(str) - 1), str + 22);
    EXPECT_PTREQ(xmemrchr(str, '1', sizeof(str) - 1), str + sizeof(str) - 2);
    EXPECT_PTREQ(xmemrchr(str, '1', sizeof(str) - 2), str);
    EXPECT_PTREQ(xmemrchr(str, '\0', sizeof(str)), str + sizeof(str) - 1);
    EXPECT_PTREQ(xmemrchr(str, '\0', sizeof(str) - 1), NULL);
    EXPECT_PTREQ(xmemrchr(str, 'z', sizeof(str) - 1), NULL);
}

static void test_str_to_bitflags(TestContext *ctx)
{
    static const char strs[][8] = {"zero", "one", "two", "three"};
    EXPECT_UINT_EQ(STR_TO_BITFLAGS("one,three", strs, true), 1 << 1 | 1 << 3);
    EXPECT_UINT_EQ(STR_TO_BITFLAGS("two,invalid,zero", strs, true), 1 << 2 | 1 << 0);
    EXPECT_UINT_EQ(STR_TO_BITFLAGS("two,invalid,zero", strs, false), 0);
}

static void test_log_level_from_str(TestContext *ctx)
{
    EXPECT_EQ(log_level_from_str("none"), LOG_LEVEL_NONE);
    EXPECT_EQ(log_level_from_str("crit"), LOG_LEVEL_CRITICAL);
    EXPECT_EQ(log_level_from_str("error"), LOG_LEVEL_ERROR);
    EXPECT_EQ(log_level_from_str("warning"), LOG_LEVEL_WARNING);
    EXPECT_EQ(log_level_from_str("notice"), LOG_LEVEL_NOTICE);
    EXPECT_EQ(log_level_from_str("info"), LOG_LEVEL_INFO);
    EXPECT_EQ(log_level_from_str("debug"), LOG_LEVEL_DEBUG);
    EXPECT_EQ(log_level_from_str("trace"), LOG_LEVEL_TRACE);

    EXPECT_EQ(log_level_from_str("xyz"), LOG_LEVEL_INVALID);
    EXPECT_EQ(log_level_from_str(" "), LOG_LEVEL_INVALID);
    EXPECT_EQ(log_level_from_str("warn"), LOG_LEVEL_INVALID);
    EXPECT_EQ(log_level_from_str("errors"), LOG_LEVEL_INVALID);

    LogLevel default_level = log_level_default();
    EXPECT_EQ(log_level_from_str(""), default_level);
    EXPECT_EQ(log_level_from_str(NULL), default_level);
}

static void test_log_level_to_str(TestContext *ctx)
{
    EXPECT_STREQ(log_level_to_str(LOG_LEVEL_NONE), "none");
    EXPECT_STREQ(log_level_to_str(LOG_LEVEL_CRITICAL), "crit");
    EXPECT_STREQ(log_level_to_str(LOG_LEVEL_ERROR), "error");
    EXPECT_STREQ(log_level_to_str(LOG_LEVEL_WARNING), "warning");
    EXPECT_STREQ(log_level_to_str(LOG_LEVEL_NOTICE), "notice");
    EXPECT_STREQ(log_level_to_str(LOG_LEVEL_INFO), "info");
    EXPECT_STREQ(log_level_to_str(LOG_LEVEL_DEBUG), "debug");
    EXPECT_STREQ(log_level_to_str(LOG_LEVEL_TRACE), "trace");
}

static void test_timespec_subtract(TestContext *ctx)
{
    struct timespec a = {.tv_sec = 3, .tv_nsec = 5497};
    struct timespec b = {.tv_sec = 1, .tv_nsec = NS_PER_SECOND - 1};
    struct timespec r = timespec_subtract(&a, &b);
    EXPECT_EQ(r.tv_sec, 1);
    EXPECT_EQ(r.tv_nsec, 5498);

    b.tv_nsec = 501;
    r = timespec_subtract(&a, &b);
    EXPECT_EQ(r.tv_sec, 2);
    EXPECT_EQ(r.tv_nsec, 4996);
}

static void test_timespec_to_str(TestContext *ctx)
{
    char buf[TIME_STR_BUFSIZE] = "";
    struct timespec ts = {.tv_sec = 0};
    EXPECT_TRUE(timespecs_equal(&ts, &ts));

    errno = 0;
    ts.tv_nsec = NS_PER_SECOND;
    EXPECT_NULL(timespec_to_str(&ts, buf));
    EXPECT_EQ(errno, EINVAL);

    errno = 0;
    ts.tv_nsec = -1;
    EXPECT_NULL(timespec_to_str(&ts, buf));
    EXPECT_EQ(errno, EINVAL);

    // Note: $TZ is set to "UTC" in test_init()
    ts.tv_sec = 4321;
    ts.tv_nsec = 9876;
    const char *r = timespec_to_str(&ts, buf);
    EXPECT_STREQ(r, "1970-01-01 01:12:01.9876 +0000");
    EXPECT_PTREQ(r, buf);

    ts.tv_sec = -1;
    ts.tv_nsec = NS_PER_SECOND - 1;
    r = timespec_to_str(&ts, buf);
    EXPECT_STREQ(r, "1969-12-31 23:59:59.999999999 +0000");
}

static void test_progname(TestContext *ctx)
{
    const char *const args[] = {"arg0", "", NULL};
    char **arg0 = (char**)args;
    char **arg1 = arg0 + 1;
    char **arg2 = arg0 + 2;
    EXPECT_STREQ(progname(1, arg0, "1"), "arg0");
    EXPECT_STREQ(progname(1, NULL, "2"), "2");
    EXPECT_STREQ(progname(0, arg0, "3"), "3");
    EXPECT_STREQ(progname(1, arg1, "4"), "4");
    EXPECT_STREQ(progname(1, arg2, "5"), "5");
    EXPECT_STREQ(progname(0, NULL, NULL), "_PROG_");
}

static const TestEntry tests[] = {
    TEST(test_util_macros),
    TEST(test_is_power_of_2),
    TEST(test_xmalloc),
    TEST(test_xstreq),
    TEST(test_xstrrchr),
    TEST(test_str_has_strn_prefix),
    TEST(test_str_has_prefix),
    TEST(test_str_has_suffix),
    TEST(test_hex_decode),
    TEST(test_hex_encode_byte),
    TEST(test_ascii),
    TEST(test_mem_equal),
    TEST(test_mem_equal_icase),
    TEST(test_base64_decode),
    TEST(test_base64_encode_block),
    TEST(test_base64_encode_final),
    TEST(test_string),
    TEST(test_string_view),
    TEST(test_strview_has_suffix),
    TEST(test_strview_remove_matching),
    TEST(test_get_delim),
    TEST(test_get_delim_str),
    TEST(test_str_replace_byte),
    TEST(test_strn_replace_byte),
    TEST(test_string_array_concat),
    TEST(test_size_str_width),
    TEST(test_buf_parse_uintmax),
    TEST(test_buf_parse_ulong),
    TEST(test_buf_parse_size),
    TEST(test_buf_parse_hex_uint),
    TEST(test_str_to_int),
    TEST(test_str_to_size),
    TEST(test_str_to_filepos),
    TEST(test_buf_umax_to_hex_str),
    TEST(test_parse_filesize),
    TEST(test_umax_to_str),
    TEST(test_uint_to_str),
    TEST(test_ulong_to_str),
    TEST(test_buf_umax_to_str),
    TEST(test_buf_uint_to_str),
    TEST(test_buf_u8_to_str),
    TEST(test_file_permissions_to_str),
    TEST(test_human_readable_size),
    TEST(test_filesize_to_str),
    TEST(test_filesize_to_str_precise),
    TEST(test_u_char_size),
    TEST(test_u_char_width),
    TEST(test_u_to_lower),
    TEST(test_u_to_upper),
    TEST(test_u_is_lower),
    TEST(test_u_is_upper),
    TEST(test_u_is_ascii_upper),
    TEST(test_u_is_cntrl),
    TEST(test_u_is_unicode),
    TEST(test_u_is_zero_width),
    TEST(test_u_is_special_whitespace),
    TEST(test_u_is_unprintable),
    TEST(test_u_str_width),
    TEST(test_u_set_char_raw),
    TEST(test_u_set_char),
    TEST(test_u_make_printable),
    TEST(test_u_get_char),
    TEST(test_u_prev_char),
    TEST(test_u_skip_chars),
    TEST(test_ptr_array),
    TEST(test_ptr_array_move),
    TEST(test_ptr_array_insert),
    TEST(test_list),
    TEST(test_hashmap),
    TEST(test_hashset),
    TEST(test_intmap),
    TEST(test_next_multiple),
    TEST(test_next_pow2),
    TEST(test_popcount),
    TEST(test_ctz),
    TEST(test_ffs),
    TEST(test_lsbit),
    TEST(test_msbit),
    TEST(test_clz),
    TEST(test_umax_bitwidth),
    TEST(test_umax_count_base16_digits),
    TEST(test_path_dirname_basename),
    TEST(test_path_relative),
    TEST(test_path_slice_relative),
    TEST(test_short_filename_cwd),
    TEST(test_short_filename),
    TEST(test_path_absolute),
    TEST(test_path_join),
    TEST(test_path_parent),
    TEST(test_wrapping_increment),
    TEST(test_wrapping_decrement),
    TEST(test_saturating_increment),
    TEST(test_saturating_decrement),
    TEST(test_saturating_subtract),
    TEST(test_size_multiply_overflows),
    TEST(test_size_add_overflows),
    TEST(test_xmul),
    TEST(test_xadd),
    TEST(test_mem_intern),
    TEST(test_read_file),
    TEST(test_xfopen),
    TEST(test_xstdio),
    TEST(test_fd_set_cloexec),
    TEST(test_fd_set_nonblock),
    TEST(test_fork_exec),
    TEST(test_xmemmem),
    TEST(test_xmemrchr),
    TEST(test_str_to_bitflags),
    TEST(test_log_level_from_str),
    TEST(test_log_level_to_str),
    TEST(test_timespec_subtract),
    TEST(test_timespec_to_str),
    TEST(test_progname),
};

const TestGroup util_tests = TEST_GROUP(tests);
