#ifndef UNICODE_H
#define UNICODE_H

#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

typedef uint_least32_t CodePoint;

static inline CONST_FN bool u_is_unicode(CodePoint uch)
{
    return uch <= UINT32_C(0x10ffff);
}

static inline CONST_FN bool u_is_ctrl(CodePoint u)
{
    return u < 0x20 || u == 0x7f;
}

bool u_is_upper(CodePoint u) CONST_FN;
bool u_is_space(CodePoint u) PURE;
bool u_is_word_char(CodePoint u) CONST_FN;
bool u_is_unprintable(CodePoint u) PURE;
bool u_is_special_whitespace(CodePoint u) PURE;
unsigned int u_char_width(CodePoint uch) PURE;
CodePoint u_to_lower(CodePoint u) CONST_FN;

#endif
