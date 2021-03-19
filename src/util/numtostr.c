#include "numtostr.h"

const char *uint_to_str(unsigned int x)
{
    static char buf[DECIMAL_STR_MAX(x)];
    size_t i = sizeof(buf) - 2;
    do {
        buf[i--] = (x % 10) + '0';
    } while (x /= 10);
    return &buf[i + 1];
}

static size_t uint_count_base10_digits(unsigned int x)
{
    size_t digits = 0;
    do {
        x /= 10;
        digits++;
    } while (x);
    return digits;
}

// Write the decimal string representation of `x` into `buf`, which must
// have at least `DECIMAL_STR_MAX(unsigned int)` bytes available.
// Returns the number of bytes (digits) written.
size_t buf_uint_to_str(unsigned int x, char *buf)
{
    const size_t ndigits = uint_count_base10_digits(x);
    size_t i = ndigits;
    buf[i--] = '\0';
    do {
        unsigned char digit = x % 10;
        buf[i--] = '0' + digit;
        x -= digit;
    } while (x /= 10);
    return ndigits;
}
