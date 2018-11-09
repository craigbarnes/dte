#include <stddef.h>
#include "unicode.h"
#include "ascii.h"

typedef struct {
    CodePoint first, last;
} CodepointRange;

#include "wcwidth.c"

// TODO: format [Cf] amd unassigned ranges
static const CodepointRange unprintable[] = {
    {0x0080, 0x009f}, // C1 control characters
    {0x200b, 0x200f},
    {0x202a, 0x202e},
    {0x2060, 0x2063},
    {0xd800, 0xdfff}, // Surrogates
    {0xfeff, 0xfeff}, // Byte order mark (BOM)
};

static inline PURE bool bisearch (
    CodePoint u,
    const CodepointRange *const range,
    size_t max
) {
    size_t min = 0;
    size_t mid;

    if (u < range[0].first || u > range[max].last) {
        return false;
    }

    while (max >= min) {
        mid = (min + max) / 2;
        if (u > range[mid].last) {
            min = mid + 1;
        } else if (u < range[mid].first) {
            max = mid - 1;
        } else {
            return true;
        }
    }

    return false;
}

bool u_is_space(CodePoint u)
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

bool u_is_word_char(CodePoint u)
{
    return u >= 0x80 || is_alnum_or_underscore(u);
}

bool u_is_unprintable(CodePoint u)
{
    if (bisearch(u, unprintable, ARRAY_COUNT(unprintable) - 1)) {
        return true;
    }
    return !u_is_unicode(u);
}

bool u_is_special_whitespace(CodePoint u)
{
    // These are all indistinguishable from ASCII space in a terminal
    switch (u) {
    case 0x00a0: // No-break space (AltGr+space)
    case 0x00ad: // Soft hyphen
    case 0x2000: // En quad
    case 0x2001: // Em quad
    case 0x2002: // En space
    case 0x2003: // Em space
    case 0x2004: // 3-per-em space
    case 0x2005: // 4-per-em space
    case 0x2006: // 6-per-em space
    case 0x2007: // Figure space
    case 0x2008: // Punctuation space
    case 0x2009: // Thin space
    case 0x200a: // Hair space
    case 0x2028: // Line separator
    case 0x2029: // Paragraph separator
    case 0x202f: // Narrow no-break space
    case 0x205f: // Medium mathematical space
    case 0x2800: // Braille pattern blank
        return true;
    }
    return false;
}

bool u_is_zero_width(CodePoint u)
{
    return bisearch(u, zero_width, ARRAY_COUNT(zero_width) - 1);
}

static bool u_is_double_width(CodePoint u)
{
    return bisearch(u, double_width, ARRAY_COUNT(double_width) - 1);
}

unsigned int u_char_width(CodePoint u)
{
    if (u < 0x80) {
        if (ascii_iscntrl(u)) {
            return 2; // Rendered in caret notation (e.g. ^@)
        }
        return 1;
    } else if (u_is_unprintable(u)) {
        return 4; // Rendered as <xx>
    } else if (u_is_zero_width(u)) {
        return 0;
    } else if (u < 0x1100U) {
        return 1;
    } else if (u_is_double_width(u)) {
        return 2;
    }

    return 1;
}
