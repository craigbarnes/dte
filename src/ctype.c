/*
 * Sane locale-independent, ASCII ctype.
 *
 * No surprises, and works with signed and unsigned chars.
 *
 * Borrowed from the GIT version control system.
 */
#include "ctype.h"

enum {
	S = GIT_SPACE,
	L = GIT_LOWER,
	U = GIT_UPPER,
	D = GIT_DIGIT,
	x = GIT_LOWER | GIT_HEX_LOWER,
	X = GIT_UPPER | GIT_HEX_UPPER,
	G = GIT_GLOB_SPECIAL,	/* *, ?, [, \\ */
	R = GIT_REGEX_SPECIAL,	/* $, (, ), +, ., ^, {, | */
};

unsigned char sane_ctype[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, S, S, 0, 0, S, 0, 0,		/*   0.. 15 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		/*  16.. 31 */
	S, 0, 0, 0, R, 0, 0, 0, R, R, G, R, 0, 0, R, 0,		/*  32.. 47 */
	D, D, D, D, D, D, D, D, D, D, 0, 0, 0, 0, 0, G,		/*  48.. 63 */
	0, X, X, X, X, X, X, U, U, U, U, U, U, U, U, U,		/*  64.. 79 */
	U, U, U, U, U, U, U, U, U, U, U, G, G, 0, R, 0,		/*  80.. 95 */
	0, x, x, x, x, x, x, L, L, L, L, L, L, L, L, L,		/*  96..111 */
	L, L, L, L, L, L, L, L, L, L, L, R, R, 0, 0, 0,		/* 112..127 */
	/* Nothing in the 128.. range */
};

int hex_decode(int ch)
{
	if (sane_istest(ch, GIT_DIGIT))
		return ch - '0';
	if (sane_istest(ch, GIT_HEX_LOWER))
		return ch - 'a' + 10;
	if (sane_istest(ch, GIT_HEX_UPPER))
		return ch - 'A' + 10;
	return -1;
}
