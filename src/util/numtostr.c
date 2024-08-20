// Locale-independent integer to string conversion.
// Copyright © Craig Barnes.
// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>
#include <sys/stat.h>
#include "numtostr.h"
#include "arith.h"
#include "debug.h"

const char hextab_lower[16] = "0123456789abcdef";
const char hextab_upper[16] = "0123456789ABCDEF";

static size_t umax_count_base10_digits(uintmax_t x)
{
    size_t digits = 0;
    do {
        x /= 10;
        digits++;
    } while (x);
    return digits;
}

static size_t umax_count_base16_digits(uintmax_t x)
{
#if HAS_BUILTIN(__builtin_clzll)
    if (sizeof(x) == sizeof(long long)) {
        size_t base2_digits = BITSIZE(x) - __builtin_clzll(x + !x);
        return round_size_to_next_multiple(base2_digits, 4) / 4;
    }
#endif
    size_t digits = 0;
    do {
        x >>= 4;
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
        buf[i--] = (x % 10) + '0';
    } while (x /= 10);
    return ndigits;
}

size_t buf_umax_to_hex_str(uintmax_t x, char *buf, size_t min_digits)
{
    const size_t ndigits = MAX(min_digits, umax_count_base16_digits(x));
    BUG_ON(ndigits < 1);
    size_t i = ndigits;
    buf[i--] = '\0';
    do {
        unsigned int nibble = x & 0xF;
        buf[i] = hextab_upper[nibble];
        x >>= 4;
    } while (i--);
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

// Like buf_umax_to_str() but for uint8_t values (1-3 digits)
size_t buf_u8_to_str(uint8_t x, char *buf)
{
    size_t ndigits = 1;
    if (x >= 100) {
        buf[2] = (x % 10) + '0';
        x /= 10;
        ndigits++;
    }

    if (x >= 10) {
        buf[1] = (x % 10) + '0';
        x /= 10;
        ndigits++;
    }

    buf[0] = (x % 10) + '0';
    return ndigits;
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

#ifdef S_ISVTX
# define VTXBIT (S_ISVTX)
#else
# define VTXBIT ((mode_t)0)
#endif

/*
 * Writes the string representation of permission bits from `mode` into `buf`.
 * This follows the POSIX ls(1) format, but excludes the "is a directory"
 * clause for the T/t field (as permitted by the spec).
 *
 * See also:
 *
 * • https://pubs.opengroup.org/onlinepubs/9699919799/utilities/ls.html#tag_20_73_10
 * • https://gnu.org/software/coreutils/manual/html_node/What-information-is-listed.html#index-long-ls-format
 */
char *filemode_to_str(mode_t mode, char *buf)
{
    static const char xmap[8] = "-xSs-xTt";

    // Owner
    buf[0] = (mode & S_IRUSR) ? 'r' : '-';
    buf[1] = (mode & S_IWUSR) ? 'w' : '-';
    buf[2] = xmap[((mode & S_IXUSR) >> 6) | ((mode & S_ISUID) >> 10)];

    // Group
    buf[3] = (mode & S_IRGRP) ? 'r' : '-';
    buf[4] = (mode & S_IWGRP) ? 'w' : '-';
    buf[5] = xmap[((mode & S_IXGRP) >> 3) | ((mode & S_ISGID) >> 9)];

    // Others
    buf[6] = (mode & S_IROTH) ? 'r' : '-';
    buf[7] = (mode & S_IWOTH) ? 'w' : '-';
    buf[8] = xmap[4 + ((mode & S_IXOTH) | ((mode & VTXBIT) >> 8))];

    buf[9] = '\0';
    return buf;
}

char *human_readable_size(uintmax_t bytes, char *buf)
{
    // See note in test_human_readable_size()
    BUG_ON(bytes >= ~((UINTMAX_MAX << 1) >> 1));

    static const char suffixes[8] = "KMGTPEZY";
    uintmax_t ipart = bytes;
    size_t nshifts = 0;

    // TODO: Use stdc_leading_zeros() instead of looping?
    while (ipart >= 1024 && nshifts < ARRAYLEN(suffixes)) {
        ipart >>= 10;
        nshifts++;
    }

    uintmax_t unit = ((uintmax_t)1) << (nshifts * 10);
    uintmax_t hundredth = unit / 100;
    unsigned int fpart = 0;
    if (hundredth) {
        uintmax_t remainder = bytes & (unit - 1);
        // TODO: Use shifting here, to avoid emitting a divide instruction
        fpart = remainder / hundredth;
        if (fpart > 99) {
            ipart++;
            fpart = 0;
        }
    }

    size_t i = buf_umax_to_str(ipart, buf);

    if (fpart > 0) {
        buf[i++] = '.';
        buf[i++] = '0' + ((fpart / 10) % 10);
        buf[i++] = '0' + (fpart % 10);
    }

    if (nshifts > 0) {
        buf[i++] = ' ';
        buf[i++] = suffixes[nshifts - 1];
        buf[i++] = 'i';
        buf[i++] = 'B';
    }

    buf[i++] = '\0';
    return buf;
}
