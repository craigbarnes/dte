#ifndef UTIL_NUMTOSTR_H
#define UTIL_NUMTOSTR_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "macros.h"

enum {
    HRSIZE_MAX = DECIMAL_STR_MAX(uintmax_t) + STRLEN(".99 GiB"),
    FILESIZE_STR_MAX = HRSIZE_MAX + DECIMAL_STR_MAX(uintmax_t) + STRLEN(" ()"),
    PRECISE_FILESIZE_STR_MAX = DECIMAL_STR_MAX(uintmax_t) + STRLEN("GiB"),
};

extern const char hextable[32];

// Encode a uint8_t as a string of 2 hexadecimal digits
static inline size_t hex_encode_byte(char out[static 2], uint8_t byte)
{
    const char *hextab_lower = hextable + 16;
    out[0] = hextab_lower[byte >> 4];
    out[1] = hextab_lower[byte & 0xF];
    return 2;
}

// Encode a uint8_t as a string of 1-3 decimal digits
static inline size_t buf_u8_to_str(uint8_t x, char *buf)
{
    // Write 3 digits unconditionally, but adjust the indices to produce
    // the correct string (x=1 writes to `buf[0]` 3 times)
    size_t i = (x >= 100);
    size_t j = (x >= 10) + i;
    buf[0] = '0' + ((x / 100) % 10);
    buf[i] = '0' + ((x / 10) % 10);
    buf[j] = '0' + (x % 10);
    return j + 1;
}

size_t buf_umax_to_str(uintmax_t x, char *buf) NONNULL_ARGS;
size_t buf_umax_to_hex_str(uintmax_t x, char *buf, size_t min_digits) NONNULL_ARGS;
size_t buf_uint_to_str(unsigned int x, char *buf) NONNULL_ARGS;
const char *umax_to_str(uintmax_t x) RETURNS_NONNULL;
const char *uint_to_str(unsigned int x) RETURNS_NONNULL;
const char *ulong_to_str(unsigned long x) RETURNS_NONNULL;
char *file_permissions_to_str(mode_t mode, char buf[static 10]) NONNULL_ARGS_AND_RETURN;
char *human_readable_size(uintmax_t bytes, char buf[static HRSIZE_MAX]) NONNULL_ARGS_AND_RETURN;
char *filesize_to_str(uintmax_t bytes, char buf[static FILESIZE_STR_MAX]) NONNULL_ARGS_AND_RETURN;
char *filesize_to_str_precise(uintmax_t bytes, char buf[static PRECISE_FILESIZE_STR_MAX]) NONNULL_ARGS_AND_RETURN;

#endif
