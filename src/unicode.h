#ifndef UNICODE_H
#define UNICODE_H

#include <stdbool.h>
#include "macros.h"

static inline CONST_FN bool u_is_unicode(unsigned int uch)
{
    return uch <= 0x10ffffU;
}

static inline CONST_FN bool u_is_ctrl(unsigned int u)
{
    return u < 0x20 || u == 0x7f;
}

bool u_is_upper(unsigned int u) CONST_FN;
bool u_is_space(unsigned int u) PURE;
bool u_is_word_char(unsigned int u) CONST_FN;
bool u_is_unprintable(unsigned int u) PURE;
bool u_is_special_whitespace(unsigned int u) PURE;
int u_char_width(unsigned int uch) PURE;
unsigned int u_to_lower(unsigned int u) CONST_FN;

#endif
