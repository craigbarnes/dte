#include "ascii.h"

enum {
    S = ASCII_SPACE,
    C = ASCII_CNTRL,
    s = ASCII_SPACE | ASCII_CNTRL,
    L = ASCII_LOWER,
    U = ASCII_UPPER,
    D = ASCII_DIGIT | ASCII_XDIGIT,
    u = ASCII_UNDERSCORE,
    N = ASCII_NONASCII,
    x = ASCII_LOWER | ASCII_XDIGIT,
    X = ASCII_UPPER | ASCII_XDIGIT,
};

const uint8_t ascii_table[256] = {
    [0x00] = C, [0x01] = C, [0x02] = C, [0x03] = C, [0x04] = C,
    [0x05] = C, [0x06] = C, ['\a'] = C, ['\b'] = C, ['\t'] = s,
    ['\n'] = s, ['\v'] = C, ['\f'] = C, ['\r'] = s, [0x0E] = C,
    C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, // 0x0F .. 0x1F

    [' '] = S,
    ['_'] = u,

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

    [0x7F] = C,
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0x80 .. 0x8F
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0x90 .. 0x9F
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xA0 .. 0xAF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xB0 .. 0xBF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xC0 .. 0xCF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xD0 .. 0xDF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xE0 .. 0xEF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xF0 .. 0xFF
};
