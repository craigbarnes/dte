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
};

const unsigned char dte_ascii_table[256] = {
    ['\t'] = S, ['\n'] = S, ['\r'] = S, [' '] = S,

    ['0'] = D, ['1'] = D, ['2'] = D, ['3'] = D, ['4'] = D,
    ['5'] = D, ['6'] = D, ['7'] = D, ['8'] = D, ['9'] = D,

    ['A'] = X, ['B'] = X, ['C'] = X, ['D'] = X, ['E'] = X, ['F'] = X,
    ['G'] = U, ['H'] = U, ['I'] = U, ['J'] = U, ['K'] = U, ['L'] = U,
    ['M'] = U, ['N'] = U, ['O'] = U, ['P'] = U, ['Q'] = U, ['R'] = U,
    ['S'] = U, ['T'] = U, ['U'] = U, ['V'] = U, ['W'] = U, ['X'] = U,
    ['Y'] = U, ['Z'] = U,

    ['a'] = x, ['b'] = x, ['c'] = x, ['d'] = x, ['e'] = x, ['f'] = x,
    ['g'] = L, ['h'] = L, ['i'] = L, ['j'] = L, ['k'] = L, ['l'] = L,
    ['m'] = L, ['n'] = L, ['o'] = L, ['p'] = L, ['q'] = L, ['r'] = L,
    ['s'] = L, ['t'] = L, ['u'] = L, ['v'] = L, ['w'] = L, ['x'] = L,
    ['y'] = L, ['z'] = L,
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
