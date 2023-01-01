#include "numtostr.h"

const char hex_encode_table[16] = "0123456789abcdef";

static size_t umax_count_base10_digits(uintmax_t x)
{
    size_t digits = 0;
    do {
        x /= 10;
        digits++;
    } while (x);
    return digits;
}

// Writes the decimal string representation of `x` into `buf`,
// which must have enough space available for a known/constant
// value of `x` or `DECIMAL_STR_MAX(x)` bytes for arbitrary values.
// Returns the number of bytes (digits) written.
size_t buf_umax_to_str(uintmax_t x, char *buf)
{
    const size_t ndigits = umax_count_base10_digits(x);
    size_t i = ndigits;
    buf[i--] = '\0';
    do {
        unsigned char digit = x % 10;
        buf[i--] = '0' + digit;
        x -= digit;
    } while (x /= 10);
    return ndigits;
}

// Writes the decimal string representation of `x` into a static
// buffer and returns a pointer to it. Unlike buf_umax_to_str(),
// this can be done without counting the number of digits first,
// by beginning at the end of the buffer and returning a pointer
// to the "first" (last written) byte offset.
const char *umax_to_str(uintmax_t x)
{
    static char buf[DECIMAL_STR_MAX(x)];
    size_t i = sizeof(buf) - 2;
    do {
        buf[i--] = (x % 10) + '0';
    } while (x /= 10);
    return &buf[i + 1];
}

const char *uint_to_str(unsigned int x)
{
    return umax_to_str(x);
}

const char *ulong_to_str(unsigned long x)
{
    return umax_to_str(x);
}

size_t buf_uint_to_str(unsigned int x, char *buf)
{
    return buf_umax_to_str(x, buf);
}
