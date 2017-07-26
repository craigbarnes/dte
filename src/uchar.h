#ifndef UCHAR_H
#define UCHAR_H

#include "unicode.h"

static inline unsigned int u_char_size(unsigned int uch)
{
	if (uch <= 0x7fU)
		return 1;
	if (uch <= 0x7ffU)
		return 2;
	if (uch <= 0xffffU)
		return 3;
	if (uch <= 0x10ffffU)
		return 4;

	// Invalid byte in UTF-8 byte sequence.
	return 1;
}

static inline void u_set_ctrl(char *buf, long *idx, unsigned int u)
{
	long i = *idx;
	buf[i++] = '^';
	if (u == 0x7f)
		buf[i++] = '?';
	else
		buf[i++] = u | 0x40;
	*idx = i;
}

unsigned int u_str_width(const unsigned char *str);

unsigned int u_prev_char(const unsigned char *buf, long *idx);
unsigned int u_str_get_char(const unsigned char *str, long *idx);
unsigned int u_get_char(const unsigned char *buf, long size, long *idx);
unsigned int u_get_nonascii(const unsigned char *buf, long size, long *idx);

void u_set_char_raw(char *str, long *idx, unsigned int uch);
void u_set_char(char *str, long *idx, unsigned int uch);
void u_set_hex(char *str, long *idx, unsigned int uch);

/*
 * Total width of skipped characters is stored back to @width.
 *
 * Stored @width can be 1 more than given width if the last skipped
 * character was double width or even 3 more if the last skipped
 * character was invalid (<xx>).
 *
 * Returns number of bytes skipped.
 */
unsigned int u_skip_chars(const char *str, int *width);

int u_str_index(const char *haystack, const char *needle_lcase);

#endif
