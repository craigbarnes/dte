#include "ascii.h"

enum {
    S = ASCII_SPACE,
    C = ASCII_CNTRL,
    s = ASCII_SPACE | ASCII_CNTRL,
    L = ASCII_LOWER,
    U = ASCII_UPPER,
    D = ASCII_DIGIT,
    u = ASCII_UNDERSCORE,
    N = ASCII_NONASCII,
    R = ASCII_REGEX,
};

const uint8_t ascii_table[256] = {
    C, C, C, C, C, C, C, C, C, s, s, C, C, s, C, C, // 0x00 .. 0x0F
    C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, // 0x10 .. 0x1F
    S, 0, 0, 0, 0, 0, 0, 0, R, R, R, R, 0, 0, R, 0, // 0x20 .. 0x2F
    D, D, D, D, D, D, D, D, D, D, 0, 0, 0, 0, 0, R, // 0x30 .. 0x3F
    0, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, // 0x40 .. 0x4F
    U, U, U, U, U, U, U, U, U, U, U, R, R, 0, 0, u, // 0x50 .. 0x5F
    0, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, // 0x60 .. 0x6F
    L, L, L, L, L, L, L, L, L, L, L, R, R, 0, 0, C, // 0x70 .. 0x7F
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0x80 .. 0x8F
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0x90 .. 0x9F
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xA0 .. 0xAF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xB0 .. 0xBF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xC0 .. 0xCF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xD0 .. 0xDF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xE0 .. 0xEF
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xF0 .. 0xFF
};
