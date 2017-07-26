/*
 * Most of this code has been borrowed from the GIT version control system.
 */
#ifndef CTYPE_H
#define CTYPE_H

/* Sane ctype - no locale, and works with signed chars */
#undef isascii
#undef isspace
#undef isdigit
#undef islower
#undef isupper
#undef isalpha
#undef isupper
#undef isalnum
#undef isxdigit
#undef tolower
#undef toupper
extern unsigned char sane_ctype[256];
#define GIT_SPACE 0x01
#define GIT_DIGIT 0x02
#define GIT_LOWER 0x04
#define GIT_UPPER 0x08
#define GIT_GLOB_SPECIAL 0x10
#define GIT_REGEX_SPECIAL 0x20
#define GIT_HEX_LOWER 0x40
#define GIT_HEX_UPPER 0x80
#define sane_istest(x,mask) ((sane_ctype[(unsigned char)(x)] & (mask)) != 0)
#define isascii(x) (((x) & ~0x7f) == 0)
#define isspace(x) sane_istest(x,GIT_SPACE)
#define isdigit(x) sane_istest(x,GIT_DIGIT)
#define islower(x) sane_istest(x,GIT_LOWER)
#define isupper(x) sane_istest(x,GIT_UPPER)
#define isalpha(x) sane_istest(x,GIT_LOWER | GIT_UPPER)
#define isalnum(x) sane_istest(x,GIT_LOWER | GIT_UPPER | GIT_DIGIT)
#define isxdigit(x) sane_istext(x,GIT_DIGIT | GIT_HEX_LOWER | GIT_HEX_UPPER)
#define is_glob_special(x) sane_istest(x,GIT_GLOB_SPECIAL)
#define is_regex_special(x) sane_istest(x,GIT_GLOB_SPECIAL | GIT_REGEX_SPECIAL)
#define tolower(x) to_lower(x)
#define toupper(x) to_upper(x)

static inline int to_lower(int x)
{
	if (isupper(x))
		x |= 0x20;
	return x;
}

static inline int to_upper(int x)
{
	if (islower(x))
		x &= ~0x20;
	return x;
}

static inline int is_word_byte(unsigned char byte)
{
	return isalnum(byte) || byte == '_' || byte > 0x7f;
}

int hex_decode(int ch);

#endif
