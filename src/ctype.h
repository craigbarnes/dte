#ifndef CTYPE_H
#define CTYPE_H

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

#define CTYPE_SPACE 0x01
#define CTYPE_DIGIT 0x02
#define CTYPE_LOWER 0x04
#define CTYPE_UPPER 0x08
#define CTYPE_GLOB_SPECIAL 0x10
#define CTYPE_REGEX_SPECIAL 0x20
#define CTYPE_HEX_LOWER 0x40
#define CTYPE_HEX_UPPER 0x80

#define sane_istest(x,mask) ((sane_ctype[(unsigned char)(x)] & (mask)) != 0)
#define isascii(x) (((x) & ~0x7f) == 0)
#define isspace(x) sane_istest(x, CTYPE_SPACE)
#define isdigit(x) sane_istest(x, CTYPE_DIGIT)
#define islower(x) sane_istest(x, CTYPE_LOWER)
#define isupper(x) sane_istest(x, CTYPE_UPPER)
#define isalpha(x) sane_istest(x, CTYPE_LOWER | CTYPE_UPPER)
#define isalnum(x) sane_istest(x, CTYPE_LOWER | CTYPE_UPPER | CTYPE_DIGIT)
#define isxdigit(x) sane_istext(x, CTYPE_DIGIT | CTYPE_HEX_LOWER | CTYPE_HEX_UPPER)
#define is_glob_special(x) sane_istest(x, CTYPE_GLOB_SPECIAL)
#define is_regex_special(x) sane_istest(x, CTYPE_GLOB_SPECIAL | CTYPE_REGEX_SPECIAL)
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
