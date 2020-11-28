#include "base64.h"

enum {
    I = BASE64_INVALID,
    P = BASE64_PADDING,
};

const uint8_t base64_table[256] = {
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I, 62,  I,  I,  I, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  I,  I,  I,  P,  I,  I,
     I,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  I,  I,  I,  I,  I,
     I, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,
     I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I,  I
};
