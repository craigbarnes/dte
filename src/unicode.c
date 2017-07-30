#include "unicode.h"
#include "common.h"

struct codepoint_range {
    unsigned int first, last;
};

// All these are indistinguishable from ASCII space on terminal.
static const struct codepoint_range evil_space[] = {
    {0x00a0, 0x00a0}, // No-break space. Easy to type accidentally (AltGr+Space)
    {0x00ad, 0x00ad}, // Soft hyphen. Very very soft...
    {0x2000, 0x200a}, // Legacy spaces of varying sizes
    {0x2028, 0x2029}, // Line and paragraph separators
    {0x202f, 0x202f}, // Narrow No-Break Space
    {0x205f, 0x205f}, // Mathematical space. Proven to be correct. Legacy
    {0x2800, 0x2800}, // Braille Pattern Blank
};

static const struct codepoint_range zero_width[] = {
    {0x200b, 0x200f},
    {0x202a, 0x202e},
    {0x2060, 0x2063},
    {0xfeff, 0xfeff},
};

static const struct codepoint_range combining[] = {
    {0x0300, 0x036F}, {0x0483, 0x0486}, {0x0488, 0x0489},
    {0x0591, 0x05BD}, {0x05BF, 0x05BF}, {0x05C1, 0x05C2},
    {0x05C4, 0x05C5}, {0x05C7, 0x05C7}, {0x0600, 0x0603},
    {0x0610, 0x0615}, {0x064B, 0x065E}, {0x0670, 0x0670},
    {0x06D6, 0x06E4}, {0x06E7, 0x06E8}, {0x06EA, 0x06ED},
    {0x070F, 0x070F}, {0x0711, 0x0711}, {0x0730, 0x074A},
    {0x07A6, 0x07B0}, {0x07EB, 0x07F3}, {0x0901, 0x0902},
    {0x093C, 0x093C}, {0x0941, 0x0948}, {0x094D, 0x094D},
    {0x0951, 0x0954}, {0x0962, 0x0963}, {0x0981, 0x0981},
    {0x09BC, 0x09BC}, {0x09C1, 0x09C4}, {0x09CD, 0x09CD},
    {0x09E2, 0x09E3}, {0x0A01, 0x0A02}, {0x0A3C, 0x0A3C},
    {0x0A41, 0x0A42}, {0x0A47, 0x0A48}, {0x0A4B, 0x0A4D},
    {0x0A70, 0x0A71}, {0x0A81, 0x0A82}, {0x0ABC, 0x0ABC},
    {0x0AC1, 0x0AC5}, {0x0AC7, 0x0AC8}, {0x0ACD, 0x0ACD},
    {0x0AE2, 0x0AE3}, {0x0B01, 0x0B01}, {0x0B3C, 0x0B3C},
    {0x0B3F, 0x0B3F}, {0x0B41, 0x0B43}, {0x0B4D, 0x0B4D},
    {0x0B56, 0x0B56}, {0x0B82, 0x0B82}, {0x0BC0, 0x0BC0},
    {0x0BCD, 0x0BCD}, {0x0C3E, 0x0C40}, {0x0C46, 0x0C48},
    {0x0C4A, 0x0C4D}, {0x0C55, 0x0C56}, {0x0CBC, 0x0CBC},
    {0x0CBF, 0x0CBF}, {0x0CC6, 0x0CC6}, {0x0CCC, 0x0CCD},
    {0x0CE2, 0x0CE3}, {0x0D41, 0x0D43}, {0x0D4D, 0x0D4D},
    {0x0DCA, 0x0DCA}, {0x0DD2, 0x0DD4}, {0x0DD6, 0x0DD6},
    {0x0E31, 0x0E31}, {0x0E34, 0x0E3A}, {0x0E47, 0x0E4E},
    {0x0EB1, 0x0EB1}, {0x0EB4, 0x0EB9}, {0x0EBB, 0x0EBC},
    {0x0EC8, 0x0ECD}, {0x0F18, 0x0F19}, {0x0F35, 0x0F35},
    {0x0F37, 0x0F37}, {0x0F39, 0x0F39}, {0x0F71, 0x0F7E},
    {0x0F80, 0x0F84}, {0x0F86, 0x0F87}, {0x0F90, 0x0F97},
    {0x0F99, 0x0FBC}, {0x0FC6, 0x0FC6}, {0x102D, 0x1030},
    {0x1032, 0x1032}, {0x1036, 0x1037}, {0x1039, 0x1039},
    {0x1058, 0x1059}, {0x1160, 0x11FF}, {0x135F, 0x135F},
    {0x1712, 0x1714}, {0x1732, 0x1734}, {0x1752, 0x1753},
    {0x1772, 0x1773}, {0x17B4, 0x17B5}, {0x17B7, 0x17BD},
    {0x17C6, 0x17C6}, {0x17C9, 0x17D3}, {0x17DD, 0x17DD},
    {0x180B, 0x180D}, {0x18A9, 0x18A9}, {0x1920, 0x1922},
    {0x1927, 0x1928}, {0x1932, 0x1932}, {0x1939, 0x193B},
    {0x1A17, 0x1A18}, {0x1B00, 0x1B03}, {0x1B34, 0x1B34},
    {0x1B36, 0x1B3A}, {0x1B3C, 0x1B3C}, {0x1B42, 0x1B42},
    {0x1B6B, 0x1B73}, {0x1DC0, 0x1DCA}, {0x1DFE, 0x1DFF},
    {0x200B, 0x200F}, {0x202A, 0x202E}, {0x2060, 0x2063},
    {0x206A, 0x206F}, {0x20D0, 0x20EF}, {0x302A, 0x302F},
    {0x3099, 0x309A}, {0xA806, 0xA806}, {0xA80B, 0xA80B},
    {0xA825, 0xA826}, {0xFB1E, 0xFB1E}, {0xFE00, 0xFE0F},
    {0xFE20, 0xFE23}, {0xFEFF, 0xFEFF}, {0xFFF9, 0xFFFB},
    {0x10A01, 0x10A03}, {0x10A05, 0x10A06}, {0x10A0C, 0x10A0F},
    {0x10A38, 0x10A3A}, {0x10A3F, 0x10A3F}, {0x1D167, 0x1D169},
    {0x1D173, 0x1D182}, {0x1D185, 0x1D18B}, {0x1D1AA, 0x1D1AD},
    {0x1D242, 0x1D244}, {0xE0001, 0xE0001}, {0xE0020, 0xE007F},
    {0xE0100, 0xE01EF}
};

static inline bool in_range(unsigned int u, const struct codepoint_range *range, int count)
{
    for (int i = 0; i < count; i++) {
        if (u < range[i].first)
            return false;
        if (u <= range[i].last)
            return true;
    }
    return false;
}

static inline bool bisearch(unsigned int u, const struct codepoint_range *range, int max)
{
    int min = 0;
    int mid;

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

static bool u_is_combining(unsigned int u)
{
    return bisearch(u, combining, ARRAY_COUNT(combining) - 1);
}

int u_char_width(unsigned int u)
{
    if (unlikely(u_is_ctrl(u)))
        return 2;

    if (likely(u < 0x80))
        return 1;

    // Unprintable characters (includes invalid bytes in unicode stream) are rendered "<xx>"
    if (u_is_unprintable(u))
        return 4;

    if (u_is_combining(u))
        return 0;

    if (likely(u < 0x1100U))
        return 1;

    // Hangul Jamo initial consonants
    if (/* u >= 0x1100U && */ u <= 0x115fU)
        return 2;

    // Angle brackets
    if (u == 0x2329U || u == 0x232aU)
        return 2;

    // CJK ... Yi
    if (u >= 0x2e80U && u <= 0xa4cfU && u != 0x303fU)
        return 2;

    // Hangul Syllables
    if (u >= 0xac00U && u <= 0xd7a3U)
        return 2;

    // CJK Compatibility Ideographs
    if (u >= 0xf900U && u <= 0xfaffU)
        return 2;

    // Vertical forms
    if (u >= 0xfe10U && u <= 0xfe19U)
        return 2;

    // CJK Compatibility Forms
    if (u >= 0xfe30U && u <= 0xfe6fU)
        return 2;

    // Fullwidth Forms
    if ((u >= 0xff00U && u <= 0xff60U) || (u >= 0xffe0U && u <= 0xffe6U))
        return 2;

    // Supplementary Ideographic Plane
    if (u >= 0x20000U && u <= 0x2fffdU)
        return 2;

    // Tertiary Ideographic Plane
    if (u >= 0x30000U && u <= 0x3fffdU)
        return 2;

    return 1;
}

unsigned int u_to_lower(unsigned int u)
{
    if (u < 'A')
        return u;
    if (u <= 'Z')
        return u + 'a' - 'A';
    return u;
}
