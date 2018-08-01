#include <limits.h>
#include <string.h>
#include "strtonum.h"
#include "ascii.h"

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

bool buf_parse_long(const char *str, size_t size, size_t *posp, long *valp)
{
    size_t pos = *posp;
    size_t count = 0;
    int sign = 1;
    long val = 0;

    if (pos < size && str[pos] == '-') {
        sign = -1;
        pos++;
    }
    while (pos < size && ascii_isdigit(str[pos])) {
        long old = val;
        val *= 10;
        val += str[pos++] - '0';
        count++;
        if (val < old) {
            // Overflow
            return false;
        }
    }
    if (count == 0) {
        return false;
    }
    *posp = pos;
    *valp = sign * val;
    return true;
}

bool parse_long(const char **strp, long *valp)
{
    const char *str = *strp;
    size_t size = strlen(str);
    size_t pos = 0;

    if (buf_parse_long(str, size, &pos, valp)) {
        *strp = str + pos;
        return true;
    }
    return false;
}

bool str_to_long(const char *str, long *valp)
{
    return parse_long(&str, valp) && *str == 0;
}

bool str_to_int(const char *str, int *valp)
{
    long val;
    if (!str_to_long(str, &val) || val < INT_MIN || val > INT_MAX) {
        return false;
    }
    *valp = val;
    return true;
}
