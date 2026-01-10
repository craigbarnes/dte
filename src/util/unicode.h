#ifndef UTIL_UNICODE_H
#define UTIL_UNICODE_H

#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

// Work around some musl-targeted toolchains failing to include this
// header automatically and thus failing to define __STDC_ISO_10646__
#if HAS_INCLUDE(<stdc-predef.h>)
# include <stdc-predef.h> // NOLINT(portability-restrict-system-includes)
#endif

#if defined(WINT_MAX) && (WINT_MAX >= 0x10FFFFUL) && defined(__STDC_ISO_10646__)
# define SANE_WCTYPE 1
#endif

// The maximum Unicode codepoint allowed by RFC 3629
#define UNICODE_MAX_VALID_CODEPOINT (0x10FFFFUL)

typedef uint32_t CodePoint;

static inline bool u_is_unicode(CodePoint u)
{
    return u <= UNICODE_MAX_VALID_CODEPOINT;
}

// https://www.unicode.org/versions/latest/core-spec/chapter-3/#G2630
static inline bool u_is_surrogate(CodePoint u)
{
    return (u >= 0xD800 && u <= 0xDFFF);
}

static inline bool u_is_cntrl(CodePoint u)
{
    return (u < 0x20) || (u >= 0x7F && u <= 0x9F);
}

static inline bool u_is_ascii_upper(CodePoint u)
{
    return u - 'A' < 26;
}

#ifdef SANE_WCTYPE
    #include <wctype.h> // NOLINT(portability-restrict-system-includes)
    // NOLINTBEGIN(*-unsafe-functions)
    static inline bool u_is_lower(CodePoint u) {return iswlower((wint_t)u);}
    static inline bool u_is_upper(CodePoint u) {return iswupper((wint_t)u);}
    static inline CodePoint u_to_lower(CodePoint u) {return towlower((wint_t)u);}
    static inline CodePoint u_to_upper(CodePoint u) {return towupper((wint_t)u);}
    // NOLINTEND(*-unsafe-functions)
#else
    static inline bool u_is_lower(CodePoint u) {return (u - 'a') < 26;}
    static inline bool u_is_upper(CodePoint u) {return (u - 'A') < 26;}
    static inline CodePoint u_to_lower(CodePoint u) {return u_is_upper(u) ? u + 32 : u;}
    static inline CodePoint u_to_upper(CodePoint u) {return u_is_lower(u) ? u - 32 : u;}
#endif

bool u_is_breakable_whitespace(CodePoint u) CONST_FN;
bool u_is_word_char(CodePoint u) CONST_FN;
bool u_is_unprintable(CodePoint u);
bool u_is_special_whitespace(CodePoint u) CONST_FN;
bool u_is_zero_width(CodePoint u);
unsigned int u_char_width(CodePoint uch) CONST_FN;

#endif
