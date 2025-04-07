#include "ascii.h"

enum {
    S = ASCII_SPACE,
    C = ASCII_CNTRL,
    s = ASCII_SPACE | ASCII_CNTRL,
    L = ASCII_LOWER,
    U = ASCII_UPPER,
    D = ASCII_DIGIT,
    P = ASCII_PUNCT,
    u = ASCII_UNDERSCORE, // No ASCII_PUNCT here, to simplify is_word_byte()
    R = ASCII_REGEX | ASCII_PUNCT,
};

const uint8_t ascii_table[256] = {
    C, C, C, C, C, C, C, C, C, s, s, C, C, s, C, C, // 0x00
    C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, // 0x10
    S, P, P, P, R, P, P, P, R, R, R, R, P, P, R, P, // 0x20   !"#$%&'()*+,-./
    D, D, D, D, D, D, D, D, D, D, P, P, P, P, P, R, // 0x30  0123456789:;<=>?
    P, U, U, U, U, U, U, U, U, U, U, U, U, U, U, U, // 0x40  @ABCDEFGHIJKLMNO
    U, U, U, U, U, U, U, U, U, U, U, R, R, P, R, u, // 0x50  PQRSTUVWXYZ[\]^_
    P, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, // 0x60  `abcdefghijklmno
    L, L, L, L, L, L, L, L, L, L, L, R, R, P, P, C, // 0x70  pqrstuvwxyz{|}~
};
