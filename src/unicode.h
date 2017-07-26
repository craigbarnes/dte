#ifndef UNICODE_H
#define UNICODE_H

#include "libc.h"

static inline bool u_is_unicode(unsigned int uch)
{
	return uch <= 0x10ffffU;
}

static inline bool u_is_ctrl(unsigned int u)
{
	return u < 0x20 || u == 0x7f;
}

bool u_is_upper(unsigned int u);
bool u_is_space(unsigned int u);
bool u_is_word_char(unsigned int u);
bool u_is_unprintable(unsigned int u);
bool u_is_special_whitespace(unsigned int u);
int u_char_width(unsigned int uch);
unsigned int u_to_lower(unsigned int u);

#endif
