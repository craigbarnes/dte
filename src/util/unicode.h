#ifndef UTIL_UNICODE_H
#define UTIL_UNICODE_H

#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

#define UNICODE_MAX_VALID_CODEPOINT UINT32_C(0x10ffff)

typedef uint32_t CodePoint;

static inline bool u_is_unicode(CodePoint u)
{
    return u <= UNICODE_MAX_VALID_CODEPOINT;
}

static inline bool u_is_cntrl(CodePoint u)
{
    return (u < 0x20) || (u >= 0x7F && u <= 0x9F);
}

static inline bool u_is_upper(CodePoint u)
{
    return (u - 'A') < 26;
}

static inline CodePoint u_to_lower(CodePoint u)
{
    return u_is_upper(u) ? u + 32 : u;
}

bool u_is_breakable_whitespace(CodePoint u) CONST_FN;
bool u_is_word_char(CodePoint u) CONST_FN;
bool u_is_unprintable(CodePoint u) CONST_FN;
bool u_is_special_whitespace(CodePoint u) CONST_FN;
bool u_is_zero_width(CodePoint u);
unsigned int u_char_width(CodePoint uch) CONST_FN;

#endif
