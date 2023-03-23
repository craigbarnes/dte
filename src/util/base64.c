#include "base64.h"
#include "debug.h"

enum {
    I = BASE64_INVALID,
    P = BASE64_PADDING,
};

const uint8_t base64_decode_table[256] = {
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

const char base64_encode_table[64] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/"
};

size_t base64_encode_block(const char *in, size_t ilen, char *out, size_t olen)
{
    BUG_ON(ilen == 0);
    BUG_ON(ilen % 3 != 0);
    BUG_ON(ilen / 3 * 4 > olen);
    const unsigned char *u_in = in;
    size_t o = 0;

    for (size_t i = 0; i < ilen; ) {
        uint32_t a = u_in[i++];
        uint32_t b = u_in[i++];
        uint32_t c = u_in[i++];
        uint32_t v = a << 16 | b << 8 | c;
        out[o++] = base64_encode_table[(v >> 18) & 63];
        out[o++] = base64_encode_table[(v >> 12) & 63];
        out[o++] = base64_encode_table[(v >>  6) & 63];
        out[o++] = base64_encode_table[(v >>  0) & 63];
    }

    return o;
}

void base64_encode_final(const char *in, size_t ilen, char out[4])
{
    BUG_ON(ilen - 1 > 1);
    const unsigned char *u_in = in;
    uint32_t a = u_in[0];
    uint32_t b = (ilen == 2) ? u_in[1] : 0;
    uint32_t v = a << 16 | b << 8;
    char x = (ilen == 2) ? base64_encode_table[(v >> 6) & 63] : '=';
    out[0] = base64_encode_table[(v >> 18) & 63];
    out[1] = base64_encode_table[(v >> 12) & 63];
    out[2] = x;
    out[3] = '=';
}
