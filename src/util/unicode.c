#include <stddef.h>
#include "unicode.h"
#include "unidata.h"
#include "ascii.h"

#define BISEARCH(u, arr) bisearch((u), (arr), ARRAYLEN(arr) - 1)

static bool bisearch(CodePoint u, const CodepointRange *range, size_t max)
{
    if (u < range[0].first || u > range[max].last) {
        return false;
    }

    size_t min = 0;
    while (max >= min) {
        const size_t mid = (min + max) / 2;
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

// Returns true for any whitespace character that isn't "non-breaking",
// i.e. one that is used purely to separate words and may, for example,
// be "broken" (changed to a newline) by hard wrapping.
bool u_is_breakable_whitespace(CodePoint u)
{
    switch (u) {
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
    case 0x1680: // Ogham space mark
    case 0x2000: // En quad
    case 0x2001: // Em quad
    case 0x2002: // En space
    case 0x2003: // Em space
    case 0x2004: // 3-per-em space
    case 0x2005: // 4-per-em space
    case 0x2006: // 6-per-em space
    case 0x2008: // Punctuation space
    case 0x2009: // Thin space
    case 0x200A: // Hair space
    case 0x200B: // Zero width space
    case 0x205F: // Medium mathematical space
    case 0x3000: // Ideographic space
        return true;
    }
    return false;
}

bool u_is_word_char(CodePoint u)
{
    return u >= 0x80 || is_alnum_or_underscore(u);
}

static bool u_is_default_ignorable(CodePoint u)
{
    return BISEARCH(u, default_ignorable);
}

bool u_is_unprintable(CodePoint u)
{
    return BISEARCH(u, unprintable) || !u_is_unicode(u);
}

bool u_is_special_whitespace(CodePoint u)
{
    return BISEARCH(u, special_whitespace);
}

static bool u_is_nonspacing_mark(CodePoint u)
{
    return BISEARCH(u, nonspacing_mark);
}

bool u_is_zero_width(CodePoint u)
{
    return u_is_nonspacing_mark(u) || u_is_default_ignorable(u);
}

static bool u_is_double_width(CodePoint u)
{
    return BISEARCH(u, double_width);
}

// Get the display width of `u`, where "display width" means the number
// of terminal columns occupied (either by the terminal's rendered font
// glyph or our own multi-column rendering)
unsigned int u_char_width(CodePoint u)
{
    if (likely(u < 0x80)) {
        if (unlikely(ascii_iscntrl(u))) {
            return 2; // Rendered by u_set_char() in caret notation (e.g. ^@)
        }
        return 1;
    } else if (u_is_zero_width(u)) {
        return 0;
    } else if (u_is_unprintable(u)) {
        return 4; // Rendered by u_set_char() as <xx>
    } else if (u < 0x1100) {
        return 1;
    } else if (u_is_double_width(u)) {
        return 2; // Rendered by (modern) terminals as a 2 column glyph (e.g. 🎧)
    }

    return 1;
}
