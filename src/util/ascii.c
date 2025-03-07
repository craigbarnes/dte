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
    C, C, C, C, C, C, C, C, C, s, s, C, C, s, C, C, // 0x00
    C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, // 0x10
    S, 0, 0, 0, R, 0, 0, 0, R, R, R, R, 0, 0, R, 0, // 0x20   !"#$%&'()*+,-./
    D, D, D, D, D, D, D, D, D, D, 0, 0, 0, 0, 0, R, // 0x30  0123456789:;<=>?
    0, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, // 0x40  @ABCDEFGHIJKLMNO
    U, U, U, U, U, U, U, U, U, U, U, R, R, 0, R, u, // 0x50  PQRSTUVWXYZ[\]^_
    0, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, // 0x60  `abcdefghijklmno
    L, L, L, L, L, L, L, L, L, L, L, R, R, 0, 0, C, // 0x70  pqrstuvwxyz{|}~
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0x80
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0x90
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xA0
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xB0
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xC0
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xD0
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xE0
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, // 0xF0
};
