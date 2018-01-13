// Sane, locale-independent ASCII ctype (borrowed from git).
// No surprises and works with signed and unsigned chars.

#include "ascii.h"

enum {
    S = ASCII_SPACE,
    L = ASCII_LOWER,
    U = ASCII_UPPER,
    D = ASCII_DIGIT,
    x = ASCII_LOWER | ASCII_HEX_LOWER,
    X = ASCII_UPPER | ASCII_HEX_UPPER,
    G = ASCII_GLOB_SPECIAL, // *, ?, \\, [
    R = ASCII_REGEX_SPECIAL, // $, (, ), +, ., ^, {, |
};

const unsigned char dte_ascii_table[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, S, S, 0, 0, S, 0, 0, //   0.. 15
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //  16.. 31
    S, 0, 0, 0, R, 0, 0, 0, R, R, G, R, 0, 0, R, 0, //  32.. 47
    D, D, D, D, D, D, D, D, D, D, 0, 0, 0, 0, 0, G, //  48.. 63
    0, X, X, X, X, X, X, U, U, U, U, U, U, U, U, U, //  64.. 79
    U, U, U, U, U, U, U, U, U, U, U, G, G, 0, R, 0, //  80.. 95
    0, x, x, x, x, x, x, L, L, L, L, L, L, L, L, L, //  96..111
    L, L, L, L, L, L, L, L, L, L, L, R, R, 0, 0, 0, // 112..127
    // Nothing in the 128.. range
};

int hex_decode(int ch)
{
    if (sane_istest(ch, ASCII_DIGIT)) {
        return ch - '0';
    } else if (sane_istest(ch, ASCII_HEX_LOWER)) {
        return ch - 'a' + 10;
    } else if (sane_istest(ch, ASCII_HEX_UPPER)) {
        return ch - 'A' + 10;
    }
    return -1;
}
