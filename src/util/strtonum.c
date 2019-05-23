#include <limits.h>
#include <string.h>
#include "strtonum.h"
#include "ascii.h"
#include "checked-arith.h"

int number_width(long n)
{
    int width = 0;

    if (n < 0) {
        n *= -1;
        width++;
    }
    do {
        n /= 10;
        width++;
    } while (n);
    return width;
}

size_t buf_parse_uintmax(const char *str, size_t size, uintmax_t *valp)
{
    if (size == 0 || !ascii_isdigit(str[0])) {
        return 0;
    }

    uintmax_t val = str[0] - '0';
    size_t i = 1;

    while (i < size && ascii_isdigit(str[i])) {
        if (umax_multiply_overflows(val, 10, &val)) {
            return 0;
        }
        if (umax_add_overflows(val, str[i++] - '0', &val)) {
            return 0;
        }
    }

    *valp = val;
    return i;
}

size_t buf_parse_ulong(const char *str, size_t size, unsigned long *valp)
{
    uintmax_t val;
    size_t n = buf_parse_uintmax(str, size, &val);
    if (n == 0 || val > ULONG_MAX) {
        return 0;
    }
    *valp = (unsigned long)val;
    return n;
}

size_t buf_parse_uint(const char *str, size_t size, unsigned int *valp)
{
    uintmax_t val;
    size_t n = buf_parse_uintmax(str, size, &val);
    if (n == 0 || val > UINT_MAX) {
        return 0;
    }
    *valp = (unsigned int)val;
    return n;
}

static size_t buf_parse_long(const char *str, size_t size, long *valp)
{
    bool negative = false;
    size_t skipped = 0;
    if (size == 0) {
        return 0;
    } else {
        switch (str[0]) {
        case '-':
            negative = true;
            // Fallthrough
        case '+':
            skipped = 1;
            str++;
            size--;
            break;
        }
    }

    uintmax_t val;
    size_t n = buf_parse_uintmax(str, size, &val);
    if (n == 0 || val > LONG_MAX) {
        return 0;
    }

    if (negative) {
        *valp = -((long)val);
    } else {
        *valp = (long)val;
    }

    return n + skipped;
}

bool str_to_int(const char *str, int *valp)
{
    const size_t len = strlen(str);
    if (len == 0) {
        return false;
    }
    long val;
    const size_t n = buf_parse_long(str, len, &val);
    if (n != len || val < INT_MIN || val > INT_MAX) {
        return false;
    }
    *valp = (int)val;
    return true;
}

bool str_to_uint(const char *str, unsigned int *valp)
{
    const size_t len = strlen(str);
    if (len == 0) {
        return false;
    }
    uintmax_t val;
    const size_t n = buf_parse_uintmax(str, len, &val);
    if (n != len || val > UINT_MAX) {
        return false;
    }
    *valp = (unsigned int)val;
    return true;
}

bool str_to_size(const char *str, size_t *valp)
{
    const size_t len = strlen(str);
    if (len == 0) {
        return false;
    }
    uintmax_t val;
    const size_t n = buf_parse_uintmax(str, len, &val);
    if (n != len || val > SIZE_MAX) {
        return false;
    }
    *valp = (size_t)val;
    return true;
}

bool str_to_ulong(const char *str, unsigned long *valp)
{
    const size_t len = strlen(str);
    if (len == 0) {
        return false;
    }
    unsigned long val;
    const size_t n = buf_parse_ulong(str, len, &val);
    if (n != len) {
        return false;
    }
    *valp = val;
    return true;
}
