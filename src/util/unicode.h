#ifndef UTIL_UNICODE_H
#define UTIL_UNICODE_H

#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

#define UNICODE_MAX_VALID_CODEPOINT UINT32_C(0x10ffff)

typedef uint32_t CodePoint;

static inline CONST_FN bool u_is_unicode(CodePoint u)
{
    return u <= UNICODE_MAX_VALID_CODEPOINT;
}

static inline CONST_FN bool u_is_upper(CodePoint u)
{
    return (u - 'A') < 26;
}

static inline CONST_FN CodePoint u_to_lower(CodePoint u)
{
    return u_is_upper(u) ? u + 32 : u;
}

bool u_is_space(CodePoint u) CONST_FN;
bool u_is_word_char(CodePoint u) PURE;
bool u_is_unprintable(CodePoint u) PURE;
bool u_is_special_whitespace(CodePoint u) CONST_FN;
bool u_is_zero_width(CodePoint u) PURE;
unsigned int u_char_width(CodePoint uch) PURE;

#endif
