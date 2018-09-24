#include <stddef.h>
#include "unicode.h"
#include "ascii.h"

typedef struct {
    CodePoint first, last;
} CodepointRange;

#include "../lookup/wcwidth.c"

// All these are indistinguishable from ASCII space on terminal.
static const CodepointRange evil_space[] = {
    {0x00a0, 0x00a0}, // No-break space. Easy to type accidentally (AltGr+Space)
    {0x00ad, 0x00ad}, // Soft hyphen. Very very soft...
    {0x2000, 0x200a}, // Legacy spaces of varying sizes
    {0x2028, 0x2029}, // Line and paragraph separators
    {0x202f, 0x202f}, // Narrow No-Break Space
    {0x205f, 0x205f}, // Mathematical space. Proven to be correct. Legacy
    {0x2800, 0x2800}, // Braille Pattern Blank
};

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

bool u_is_upper(CodePoint u)
{
    return u >= 'A' && u <= 'Z';
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
    if (u > 0x7f) {
        return true;
    }
    return ascii_isalnum(u) || u == '_';
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
    return bisearch(u, evil_space, ARRAY_COUNT(evil_space) - 1);
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
#if GNUC_AT_LEAST(3, 0)
    switch (u) {
    // C0 control or DEL (rendered in caret notation)
    case 0x00 ... 0x1F:
    case 0x7F:
        return 2;
    // Printable ASCII
    case 0x20 ... 0x7E:
        return 1;
    }
#else
    if (u_is_ctrl(u)) {
        return 2;
    } else if (u <= 0x7E) {
        return 1;
    }
#endif

    if (u_is_unprintable(u)) {
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

CodePoint u_to_lower(CodePoint u)
{
    if (u < 'A') {
        return u;
    }
    if (u <= 'Z') {
        return u + 'a' - 'A';
    }
    return u;
}
