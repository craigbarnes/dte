#if defined(unix) || defined(__unix__) || defined(__unix)
# define PREDEF_PLATFORM_UNIX
# define _XOPEN_SOURCE
#endif

#if defined(PREDEF_PLATFORM_UNIX)
# include <unistd.h>
# if defined(_XOPEN_VERSION)
#  if (_XOPEN_VERSION >= 4)
#   define HAVE_WCWIDTH 1
#  endif
# endif
#endif

#if defined(HAVE_WCWIDTH)
# include <wchar.h>
# if (WCHAR_MAX >= 0x10FFFF)
#  define HAVE_SANE_WCWIDTH 1
# endif
#endif

#include "unicode.h"
#include "common.h"

struct codepoint_range {
	unsigned int lo, hi;
};

// All these are indistinguishable from ASCII space on terminal.
static const struct codepoint_range evil_space[] = {
	{ 0x00a0, 0x00a0 }, // No-break space. Easy to type accidentally (AltGr+Space)
	{ 0x00ad, 0x00ad }, // Soft hyphen. Very very soft...
	{ 0x2000, 0x200a }, // Legacy spaces of varying sizes
	{ 0x2028, 0x2029 }, // Line and paragraph separators
	{ 0x202f, 0x202f }, // Narrow No-Break Space
	{ 0x205f, 0x205f }, // Mathematical space. Proven to be correct. Legacy
	{ 0x2800, 0x2800 }, // Braille Pattern Blank
};

static const struct codepoint_range zero_width[] = {
	{ 0x200b, 0x200f },
	{ 0x202a, 0x202e },
	{ 0x2060, 0x2063 },
	{ 0xfeff, 0xfeff },
};

static inline bool in_range(unsigned int u, const struct codepoint_range *range, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (u < range[i].lo)
			return false;
		if (u <= range[i].hi)
			return true;
	}
	return false;
}

bool u_is_upper(unsigned int u)
{
	return u >= 'A' && u <= 'Z';
}

bool u_is_space(unsigned int u)
{
	switch (u) {
	case '\t':
	case '\n':
	case '\v':
	case '\f':
	case '\r':
	case ' ':
		return true;
	}
	return u_is_special_whitespace(u);
}

bool u_is_word_char(unsigned int u)
{
	if (u >= 'a' && u <= 'z')
		return true;
	if (u >= 'A' && u <= 'Z')
		return true;
	if (u >= '0' && u <= '9')
		return true;
	return u == '_' || u > 0x7f;
}

bool u_is_unprintable(unsigned int u)
{
	// Unprintable garbage inherited from latin1.
	if (u >= 0x80 && u <= 0x9f)
		return true;

	if (in_range(u, zero_width, ARRAY_COUNT(zero_width)))
		return true;

	return !u_is_unicode(u);
}

bool u_is_special_whitespace(unsigned int u)
{
	return in_range(u, evil_space, ARRAY_COUNT(evil_space));
}

#ifdef HAVE_SANE_WCWIDTH

int u_char_width(unsigned int u)
{
	int width = wcwidth((wchar_t)u);
	// Unprintable characters cause wcwidth() to return -1, but are
	// rendered as "<xx>"
	return (width >= 0) ? width : 4;
}

#else

// FIXME: incomplete. use generated tables
static bool u_is_combining(unsigned int u)
{
	return u >= 0x0300 && u <= 0x036f;
}

int u_char_width(unsigned int u)
{
	if (unlikely(u_is_ctrl(u)))
		return 2;

	if (likely(u < 0x80))
		return 1;

	/* unprintable characters (includes invalid bytes in unicode stream) are rendered "<xx>" */
	if (u_is_unprintable(u))
		return 4;

	// FIXME: hack
	if (u_is_combining(u))
		return 0;

	if (likely(u < 0x1100U))
		return 1;

	/* Hangul Jamo init. consonants */
	if (u <= 0x115fU)
		goto wide;

	/* angle brackets */
	if (u == 0x2329U || u == 0x232aU)
		goto wide;

	if (u < 0x2e80U)
		goto narrow;
	/* CJK ... Yi */
	if (u < 0x302aU)
		goto wide;
	if (u <= 0x302fU)
		goto narrow;
	if (u == 0x303fU)
		goto narrow;
	if (u == 0x3099U)
		goto narrow;
	if (u == 0x309aU)
		goto narrow;
	/* CJK ... Yi */
	if (u <= 0xa4cfU)
		goto wide;

	/* Hangul Syllables */
	if (u >= 0xac00U && u <= 0xd7a3U)
		goto wide;

	/* CJK Compatibility Ideographs */
	if (u >= 0xf900U && u <= 0xfaffU)
		goto wide;

	/* CJK Compatibility Forms */
	if (u >= 0xfe30U && u <= 0xfe6fU)
		goto wide;

	/* Fullwidth Forms */
	if (u >= 0xff00U && u <= 0xff60U)
		goto wide;

	/* Fullwidth Forms */
	if (u >= 0xffe0U && u <= 0xffe6U)
		goto wide;

	/* CJK extra stuff */
	if (u >= 0x20000U && u <= 0x2fffdU)
		goto wide;

	/* ? */
	if (u >= 0x30000U && u <= 0x3fffdU)
		goto wide;
narrow:
	return 1;
wide:
	return 2;
}

#endif

unsigned int u_to_lower(unsigned int u)
{
	if (u < 'A')
		return u;
	if (u <= 'Z')
		return u + 'a' - 'A';
	return u;
}
