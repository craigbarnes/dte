#include "test.h"
#include "../src/util/ascii.h"

void test_util_ascii(void)
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

    EXPECT_EQ(ascii_isspace(' '), true);
    EXPECT_EQ(ascii_isspace('\t'), true);
    EXPECT_EQ(ascii_isspace('\r'), true);
    EXPECT_EQ(ascii_isspace('\n'), true);

    EXPECT_EQ(is_word_byte('a'), true);
    EXPECT_EQ(is_word_byte('z'), true);
    EXPECT_EQ(is_word_byte('A'), true);
    EXPECT_EQ(is_word_byte('Z'), true);
    EXPECT_EQ(is_word_byte('0'), true);
    EXPECT_EQ(is_word_byte('9'), true);
    EXPECT_EQ(is_word_byte('_'), true);

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
