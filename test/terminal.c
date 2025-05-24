#include "test.h"
#include "terminal/color.h"
#include "terminal/cursor.h"
#include "terminal/input.h"
#include "terminal/key.h"
#include "terminal/linux.h"
#include "terminal/osc52.h"
#include "terminal/output.h"
#include "terminal/parse.h"
#include "terminal/rxvt.h"
#include "terminal/style.h"
#include "terminal/terminal.h"
#include "ui.h" // update_term_title()
#include "util/bit.h"
#include "util/str-array.h"
#include "util/unicode.h"
#include "util/utf8.h"
#include "util/xsnprintf.h"

#define TFLAG(flags) (KEYCODE_QUERY_REPLY_BIT | (flags))
#define IEXPECT_KEYCODE_EQ(a, b, seq, seq_len) IEXPECT(keycode_eq, a, b, seq, seq_len)
#define EXPECT_PARSE_SEQ(seq, expkey) EXPECT(parse_seq, seq, STRLEN(seq), expkey)
#define EXPECT_PARSE_SEQN(seq, explen, expkey) EXPECT(parse_seq, seq, explen, expkey)

static void iexpect_keycode_eq (
    TestContext *ctx,
    const char *file,
    int line,
    size_t idx,
    KeyCode a,
    KeyCode b,
    const char *seq,
    size_t seq_len
) {
    if (likely(a == b)) {
        test_pass(ctx);
        return;
    }

    char a_str[KEYCODE_STR_BUFSIZE];
    char b_str[KEYCODE_STR_BUFSIZE];
    char seq_str[64];
    keycode_to_str(a, a_str);
    keycode_to_str(b, b_str);
    u_make_printable(seq, seq_len, seq_str, sizeof seq_str, 0);

    test_fail(
        ctx, file, line,
        "Test #%zu: key codes not equal: 0x%02x, 0x%02x (%s, %s); input: %s",
        ++idx, a, b, a_str, b_str, seq_str
    );
}

static void expect_parse_seq (
    TestContext *ctx,
    const char *file,
    int line,
    const char *seq,
    ssize_t expected_length,
    KeyCode expected_key
) {
    KeyCode key = 0x18;
    ssize_t parsed_length = term_parse_sequence(seq, strlen(seq), &key);
    if (unlikely(parsed_length != expected_length)) {
        test_fail (
            ctx, file, line,
            "term_parse_sequence() returned %zd; expected %zd",
            parsed_length, expected_length
        );
        return;
    }

    static_assert(TPARSE_PARTIAL_MATCH == -1);
    if (expected_length <= 0) {
        test_pass(ctx);
        return;
    }

    if (unlikely(key != expected_key)) {
        char str[2][KEYCODE_STR_BUFSIZE];
        keycode_to_str(key, str[0]);
        keycode_to_str(expected_key, str[1]);
        test_fail (
            ctx, file, line,
            "Key codes not equal: 0x%02x, 0x%02x (%s, %s)",
            key, expected_key, str[0], str[1]
        );
        return;
    }

    for (size_t n = expected_length - 1; n != 0; n--) {
        parsed_length = term_parse_sequence(seq, n, &key);
        if (parsed_length == TPARSE_PARTIAL_MATCH) {
            continue;
        }
        test_fail (
            ctx, file, line,
            "Parsing truncated sequence of %zu bytes returned %zd; expected -1",
            n, parsed_length
        );
        return;
    }

    test_pass(ctx);
}

static void test_parse_rgb(TestContext *ctx)
{
    EXPECT_EQ(parse_rgb(STRN("f01cff")), COLOR_RGB(0xf01cff));
    EXPECT_EQ(parse_rgb(STRN("011011")), COLOR_RGB(0x011011));
    EXPECT_EQ(parse_rgb(STRN("12aC90")), COLOR_RGB(0x12ac90));
    EXPECT_EQ(parse_rgb(STRN("fffffF")), COLOR_RGB(0xffffff));
    EXPECT_EQ(parse_rgb(STRN("fa0")), COLOR_RGB(0xffaa00));
    EXPECT_EQ(parse_rgb(STRN("123")), COLOR_RGB(0x112233));
    EXPECT_EQ(parse_rgb(STRN("fff")), COLOR_RGB(0xffffff));
    EXPECT_EQ(parse_rgb(STRN("ABC")), COLOR_RGB(0xaabbcc));
    EXPECT_EQ(parse_rgb(STRN("0F0")), COLOR_RGB(0x00ff00));
    EXPECT_EQ(parse_rgb(STRN("000")), COLOR_RGB(0x000000));
    EXPECT_EQ(parse_rgb(STRN("fffffg")), COLOR_INVALID);
    EXPECT_EQ(parse_rgb(STRN(".")), COLOR_INVALID);
    EXPECT_EQ(parse_rgb(STRN("")), COLOR_INVALID);
    EXPECT_EQ(parse_rgb(STRN("11223")), COLOR_INVALID);
    EXPECT_EQ(parse_rgb(STRN("efg")), COLOR_INVALID);
    EXPECT_EQ(parse_rgb(STRN("12")), COLOR_INVALID);
    EXPECT_EQ(parse_rgb(STRN("ffff")), COLOR_INVALID);
    EXPECT_EQ(parse_rgb(STRN("00")), COLOR_INVALID);
    EXPECT_EQ(parse_rgb(STRN("1234567")), COLOR_INVALID);
    EXPECT_EQ(parse_rgb(STRN("123456789")), COLOR_INVALID);
}

static void test_parse_term_style(TestContext *ctx)
{
    static const struct {
        ssize_t expected_return;
        const char *const strs[4];
        TermStyle expected_style;
    } tests[] = {
        {3, {"bold", "red", "yellow"}, {COLOR_RED, COLOR_YELLOW, ATTR_BOLD}},
        {1, {"#ff0000"}, {COLOR_RGB(0xff0000), -1, 0}},
        {2, {"#f00a9c", "reverse"}, {COLOR_RGB(0xf00a9c), -1, ATTR_REVERSE}},
        {2, {"black", "#00ffff"}, {COLOR_BLACK, COLOR_RGB(0x00ffff), 0}},
        {2, {"#123456", "#abcdef"}, {COLOR_RGB(0x123456), COLOR_RGB(0xabcdef), 0}},
        {2, {"#123", "#fa0"}, {COLOR_RGB(0x112233), COLOR_RGB(0xffaa00), 0}},
        {2, {"#A1B2C3", "gray"}, {COLOR_RGB(0xa1b2c3), COLOR_GRAY, 0}},
        {2, {"red", "strikethrough"}, {COLOR_RED, -1, ATTR_STRIKETHROUGH}},
        {1, {"5/5/5"}, {231, COLOR_DEFAULT, 0}},
        {3, {"1/3/0", "0/5/2", "italic"}, {70, 48, ATTR_ITALIC}},
        {2, {"-1", "-2"}, {COLOR_DEFAULT, COLOR_KEEP, 0}},
        {3, {"keep", "red", "keep"}, {-2, COLOR_RED, ATTR_KEEP}},
        {2, {"bold", "blink"}, {-1, -1, ATTR_BOLD | ATTR_BLINK}},
        {3, {"0", "255", "underline"}, {COLOR_BLACK, 255, ATTR_UNDERLINE}},
        {3, {"white", "green", "dim"}, {COLOR_WHITE, COLOR_GREEN, ATTR_DIM}},
        {3, {"white", "green", "lowintensity"}, {COLOR_WHITE, COLOR_GREEN, ATTR_DIM}},
        {2, {"lightred", "lightyellow"}, {COLOR_LIGHTRED, COLOR_LIGHTYELLOW, 0}},
        {2, {"darkgray", "lightgreen"}, {COLOR_DARKGRAY, COLOR_LIGHTGREEN, 0}},
        {2, {"lightblue", "lightcyan"}, {COLOR_LIGHTBLUE, COLOR_LIGHTCYAN, 0}},
        {1, {"lightmagenta"}, {COLOR_LIGHTMAGENTA, COLOR_DEFAULT, 0}},
        {3, {"keep", "254", "keep"}, {COLOR_KEEP, 254, ATTR_KEEP}},
        {2, {"keep", "keep"}, {COLOR_KEEP, COLOR_KEEP, 0}},
        {3, {"red", "green", "keep"}, {COLOR_RED, COLOR_GREEN, ATTR_KEEP}},
        {3, {"1", "2", "invisible"}, {COLOR_RED, COLOR_GREEN, ATTR_INVIS}},
        {2, {"default", "0/0/0"}, {COLOR_DEFAULT, 16, 0}},
        {2, {"2/3/4", "5/5/5"}, {110, 231, 0}},
        {0, {NULL}, {COLOR_DEFAULT, COLOR_DEFAULT, 0}},
        // Invalid:
        {2, {"red", "blue", "invalid"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {-1, {"cyan", "magenta", "yellow"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"invalid", "default", "bold"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {1, {"italic", "invalid"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {2, {"red", "blue", ""}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {2, {"24", "#abc", "dims"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {""}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"."}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"#"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"-3"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"256"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"brightred"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"lighttblack"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"lightwhite"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"#fffffg"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"#12345"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"123456"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"//0/0"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"0/0/:"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"light_"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"\xFF/0/0"}, {COLOR_INVALID, COLOR_INVALID, 0}},
        {0, {"1/2/\x9E"}, {COLOR_INVALID, COLOR_INVALID, 0}},
    };
    FOR_EACH_I(i, tests) {
        const TermStyle expected = tests[i].expected_style;
        TermStyle parsed = {COLOR_INVALID, COLOR_INVALID, 0};
        char **strs = (char**)tests[i].strs;
        ssize_t n = parse_term_style(&parsed, strs, string_array_length(strs));
        IEXPECT_EQ(n, tests[i].expected_return);
        IEXPECT_EQ(parsed.fg, expected.fg);
        IEXPECT_EQ(parsed.bg, expected.bg);
        IEXPECT_EQ(parsed.attr, expected.attr);
        IEXPECT_TRUE(same_style(&parsed, &expected));
    }
}

static void test_color_to_nearest(TestContext *ctx)
{
    static const struct {
        int32_t input;
        int32_t expected_rgb;
        int32_t expected_256;
        int32_t expected_16;
    } tests[] = {
        // ECMA-48 colors
        {0, 0, 0, 0},
        {5, 5, 5, 5},
        {7, 7, 7, 7},

        // aixterm-style colors
        {8, 8, 8, 8},
        {10, 10, 10, 10},
        {15, 15, 15, 15},

        // xterm 256 palette colors
        {25, 25, 25, COLOR_BLUE},
        {87, 87, 87, COLOR_LIGHTCYAN},
        {88, 88, 88, COLOR_RED},
        {90, 90, 90, COLOR_MAGENTA},
        {96, 96, 96, COLOR_MAGENTA},
        {178, 178, 178, COLOR_YELLOW},
        {179, 179, 179, COLOR_YELLOW},

        // RGB colors with exact xterm 6x6x6 cube equivalents
        {COLOR_RGB(0x000000),  16,  16, COLOR_BLACK},
        {COLOR_RGB(0x000087),  18,  18, COLOR_BLUE},
        {COLOR_RGB(0x0000FF),  21,  21, COLOR_LIGHTBLUE},
        {COLOR_RGB(0x00AF87),  36,  36, COLOR_GREEN},
        {COLOR_RGB(0x00FF00),  46,  46, COLOR_LIGHTGREEN},
        {COLOR_RGB(0x870000),  88,  88, COLOR_RED},
        {COLOR_RGB(0xFF0000), 196, 196, COLOR_LIGHTRED},
        {COLOR_RGB(0xFFD700), 220, 220, COLOR_YELLOW},
        {COLOR_RGB(0xFFFF5F), 227, 227, COLOR_LIGHTYELLOW},
        {COLOR_RGB(0xFFFFFF), 231, 231, COLOR_WHITE},

        // RGB colors with exact xterm grayscale equivalents
        {COLOR_RGB(0x080808), 232, 232, COLOR_BLACK},
        {COLOR_RGB(0x121212), 233, 233, COLOR_BLACK},
        {COLOR_RGB(0x6C6C6C), 242, 242, COLOR_DARKGRAY},
        {COLOR_RGB(0xA8A8A8), 248, 248, COLOR_GRAY},
        {COLOR_RGB(0xB2B2B2), 249, 249, COLOR_GRAY},
        {COLOR_RGB(0xBCBCBC), 250, 250, COLOR_WHITE},
        {COLOR_RGB(0xEEEEEE), 255, 255, COLOR_WHITE},

        // RGB colors with NO exact xterm equivalents
        {COLOR_RGB(0x00FF88), COLOR_RGB(0x00FF88),  48, COLOR_LIGHTGREEN},
        {COLOR_RGB(0xFF0001), COLOR_RGB(0xFF0001), 196, COLOR_LIGHTRED},
        {COLOR_RGB(0xAABBCC), COLOR_RGB(0xAABBCC), 146, COLOR_LIGHTBLUE},
        {COLOR_RGB(0x010101), COLOR_RGB(0x010101),  16, COLOR_BLACK},
        {COLOR_RGB(0x070707), COLOR_RGB(0x070707), 232, COLOR_BLACK},
        {COLOR_RGB(0x080809), COLOR_RGB(0x080809), 232, COLOR_BLACK},
        {COLOR_RGB(0xBABABA), COLOR_RGB(0xBABABA), 250, COLOR_WHITE},
        {COLOR_RGB(0xEEEEED), COLOR_RGB(0xEEEEED), 255, COLOR_WHITE},
        {COLOR_RGB(0xEFEFEF), COLOR_RGB(0xEFEFEF), 255, COLOR_WHITE},
        {COLOR_RGB(0xF0F0F0), COLOR_RGB(0xF0F0F0), 255, COLOR_WHITE},
        {COLOR_RGB(0xF6F6F6), COLOR_RGB(0xF6F6F6), 255, COLOR_WHITE},
        {COLOR_RGB(0xF7F7F7), COLOR_RGB(0xF7F7F7), 231, COLOR_WHITE},
        {COLOR_RGB(0xFEFEFE), COLOR_RGB(0xFEFEFE), 231, COLOR_WHITE},
    };

    FOR_EACH_I(i, tests) {
        const int32_t c = tests[i].input;
        IEXPECT_EQ(color_to_nearest(c, TFLAG_TRUE_COLOR, false), c);
        IEXPECT_EQ(color_to_nearest(c, TFLAG_TRUE_COLOR, true), tests[i].expected_rgb);
        IEXPECT_EQ(color_to_nearest(c, TFLAG_256_COLOR, false), tests[i].expected_256);
        IEXPECT_EQ(color_to_nearest(c, TFLAG_16_COLOR, false), tests[i].expected_16);
        IEXPECT_EQ(color_to_nearest(c, TFLAG_8_COLOR, false), tests[i].expected_16 & 7);
        IEXPECT_EQ(color_to_nearest(c, 0, false), COLOR_DEFAULT);
    }

    static const uint8_t color_stops[6] = {0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff};
    size_t count = 0;

    for (size_t i = 1; i < ARRAYLEN(color_stops); i++) {
        uint8_t min = color_stops[i - 1];
        uint8_t max = color_stops[i];
        uint8_t mid = min + ((max - min) / 2);
        for (unsigned int b = min; b <= max; b++, count++) {
            int32_t orig = COLOR_RGB(b);
            int32_t nearest = color_to_nearest(orig, TFLAG_256_COLOR, false);
            size_t nearest_stop_idx = (b < mid) ? i - 1 : i;
            EXPECT_TRUE(nearest >= 16);
            EXPECT_TRUE(nearest <= 255);
            if (nearest < 232) {
                EXPECT_EQ(nearest, nearest_stop_idx + 16);
            } else {
                int32_t v = orig & ~COLOR_FLAG_RGB;
                EXPECT_TRUE(v >= 0x0D);
                EXPECT_TRUE(v <= 0x34);
                EXPECT_EQ(nearest, 232 + (((b / 3) - 3) / 10));
            }
            if (nearest == min || nearest == max) {
                EXPECT_EQ(nearest, b);
                EXPECT_EQ(nearest, color_to_nearest(orig, TFLAG_TRUE_COLOR, true));
            }
            EXPECT_EQ(nearest_stop_idx, ((uint8_t)b - (b < 75 ? 7 : 35)) / 40);
        }
    }

    EXPECT_EQ(count, 255 + ARRAYLEN(color_stops) - 1);
}

static void test_color_to_str(TestContext *ctx)
{
    static const struct {
        char expected_string[15];
        uint8_t expected_len;
        int32_t color;
    } tests[] = {
        {STRN("keep"), COLOR_KEEP},
        {STRN("default"), COLOR_DEFAULT},
        {STRN("black"), COLOR_BLACK},
        {STRN("gray"), COLOR_GRAY},
        {STRN("darkgray"), COLOR_DARKGRAY},
        {STRN("lightmagenta"), COLOR_LIGHTMAGENTA},
        {STRN("white"), COLOR_WHITE},
        {STRN("16"), 16},
        {STRN("255"), 255},
        {STRN("#abcdef"), COLOR_RGB(0xabcdef)},
        {STRN("#00001e"), COLOR_RGB(0x00001e)},
        {STRN("#000000"), COLOR_RGB(0x000000)},
        {STRN("#100001"), COLOR_RGB(0x100001)},
        {STRN("#ffffff"), COLOR_RGB(0xffffff)},
    };

    char buf[COLOR_STR_BUFSIZE] = "";
    FOR_EACH_I(i, tests) {
        int32_t color = tests[i].color;
        ASSERT_TRUE(color_is_valid(color));
        size_t len = color_to_str(buf, color);
        EXPECT_MEMEQ(buf, len, tests[i].expected_string, tests[i].expected_len);
    }
}

static void test_term_style_to_string(TestContext *ctx)
{
    static const struct {
        const char *expected_string;
        const TermStyle style;
    } tests[] = {
        {"red yellow bold", {COLOR_RED, COLOR_YELLOW, ATTR_BOLD}},
        {"#ff0000", {COLOR_RGB(0xff0000), -1, 0}},
        {"#f00a9c reverse", {COLOR_RGB(0xf00a9c), -1, ATTR_REVERSE}},
        {"black #00ffff", {COLOR_BLACK, COLOR_RGB(0x00ffff), 0}},
        {"#010900", {COLOR_RGB(0x010900), COLOR_DEFAULT, 0}},
        {"red strikethrough", {COLOR_RED, -1, ATTR_STRIKETHROUGH}},
        {"231", {231, COLOR_DEFAULT, 0}},
        {"70 48 italic", {70, 48, ATTR_ITALIC}},
        {"default keep", {COLOR_DEFAULT, COLOR_KEEP, 0}},
        {"keep red keep", {COLOR_KEEP, COLOR_RED, ATTR_KEEP}},
        {"88 16 blink bold", {88, 16, ATTR_BOLD | ATTR_BLINK}},
        {"black 255 underline", {COLOR_BLACK, 255, ATTR_UNDERLINE}},
        {"white green dim", {COLOR_WHITE, COLOR_GREEN, ATTR_DIM}},
        {"darkgray lightgreen", {COLOR_DARKGRAY, COLOR_LIGHTGREEN, 0}},
        {"lightmagenta", {COLOR_LIGHTMAGENTA, COLOR_DEFAULT, 0}},
        {"keep 254 keep", {COLOR_KEEP, 254, ATTR_KEEP}},
        {"default", {COLOR_DEFAULT, COLOR_DEFAULT, 0}},
        {"default bold", {COLOR_DEFAULT, COLOR_DEFAULT, ATTR_BOLD}},
        {"default default keep", {COLOR_DEFAULT, COLOR_DEFAULT, ATTR_KEEP}},
    };

    char buf[TERM_STYLE_BUFSIZE];
    FOR_EACH_I(i, tests) {
        const char *str = term_style_to_string(buf, &tests[i].style);
        EXPECT_STREQ(str, tests[i].expected_string);
    }

    // Ensure longest possible color string doesn't overflow the
    // static buffer in term_style_to_string()
    const TermStyle style = {
        .fg = COLOR_LIGHTMAGENTA,
        .bg = COLOR_LIGHTMAGENTA,
        .attr = ~0U
    };

    static const char expected[] =
        "lightmagenta lightmagenta "
        "keep underline reverse blink dim "
        "bold invisible italic strikethrough"
    ;

    static_assert(sizeof(expected) - 1 == 94);
    const char *str = term_style_to_string(buf, &style);
    EXPECT_STREQ(str, expected);
}

static void test_cursor_mode_from_str(TestContext *ctx)
{
    EXPECT_EQ(cursor_mode_from_str("default"), CURSOR_MODE_DEFAULT);
    EXPECT_EQ(cursor_mode_from_str("insert"), CURSOR_MODE_INSERT);
    EXPECT_EQ(cursor_mode_from_str("overwrite"), CURSOR_MODE_OVERWRITE);
    EXPECT_EQ(cursor_mode_from_str("cmdline"), CURSOR_MODE_CMDLINE);
    EXPECT_EQ(cursor_mode_from_str(""), NR_CURSOR_MODES);
    EXPECT_EQ(cursor_mode_from_str("d"), NR_CURSOR_MODES);
    EXPECT_EQ(cursor_mode_from_str("defaults"), NR_CURSOR_MODES);
    EXPECT_EQ(cursor_mode_from_str("command"), NR_CURSOR_MODES);
}

static void test_cursor_type_from_str(TestContext *ctx)
{
    EXPECT_EQ(cursor_type_from_str("default"), CURSOR_DEFAULT);
    EXPECT_EQ(cursor_type_from_str("keep"), CURSOR_KEEP);
    EXPECT_EQ(cursor_type_from_str("block"), CURSOR_STEADY_BLOCK);
    EXPECT_EQ(cursor_type_from_str("bar"), CURSOR_STEADY_BAR);
    EXPECT_EQ(cursor_type_from_str("underline"), CURSOR_STEADY_UNDERLINE);
    EXPECT_EQ(cursor_type_from_str("blinking-block"), CURSOR_BLINKING_BLOCK);
    EXPECT_EQ(cursor_type_from_str("blinking-bar"), CURSOR_BLINKING_BAR);
    EXPECT_EQ(cursor_type_from_str("blinking-underline"), CURSOR_BLINKING_UNDERLINE);
    EXPECT_EQ(cursor_type_from_str(""), CURSOR_INVALID);
    EXPECT_EQ(cursor_type_from_str("b"), CURSOR_INVALID);
    EXPECT_EQ(cursor_type_from_str("bars"), CURSOR_INVALID);
    EXPECT_EQ(cursor_type_from_str("blinking-default"), CURSOR_INVALID);
    EXPECT_EQ(cursor_type_from_str("blinking-keep"), CURSOR_INVALID);
}

static void test_cursor_color_from_str(TestContext *ctx)
{
    EXPECT_EQ(cursor_color_from_str("keep"), COLOR_KEEP);
    EXPECT_EQ(cursor_color_from_str("default"), COLOR_DEFAULT);
    EXPECT_EQ(cursor_color_from_str("#f9a"), COLOR_RGB(0xFF99AA));
    EXPECT_EQ(cursor_color_from_str("#123456"), COLOR_RGB(0x123456));
    EXPECT_EQ(cursor_color_from_str("red"), COLOR_INVALID);
    EXPECT_EQ(cursor_color_from_str(""), COLOR_INVALID);
    EXPECT_EQ(cursor_color_from_str("#"), COLOR_INVALID);
    EXPECT_EQ(cursor_color_from_str("0"), COLOR_INVALID);
    EXPECT_EQ(cursor_color_from_str("#12345"), COLOR_INVALID);
    EXPECT_EQ(cursor_color_from_str("#1234567"), COLOR_INVALID);
    EXPECT_EQ(cursor_color_from_str("123456"), COLOR_INVALID);
}

static void test_cursor_color_to_str(TestContext *ctx)
{
    char buf[COLOR_STR_BUFSIZE];
    EXPECT_STREQ(cursor_color_to_str(buf, COLOR_DEFAULT), "default");
    EXPECT_STREQ(cursor_color_to_str(buf, COLOR_KEEP), "keep");
    EXPECT_STREQ(cursor_color_to_str(buf, COLOR_RGB(0x190AFE)), "#190afe");
}

static void test_same_cursor(TestContext *ctx)
{
    TermCursorStyle a = get_default_cursor_style(CURSOR_MODE_DEFAULT);
    EXPECT_EQ(a.type, CURSOR_DEFAULT);
    EXPECT_EQ(a.color, COLOR_DEFAULT);
    EXPECT_TRUE(same_cursor(&a, &a));

    TermCursorStyle b = get_default_cursor_style(CURSOR_MODE_INSERT);
    EXPECT_EQ(b.type, CURSOR_KEEP);
    EXPECT_EQ(b.color, COLOR_KEEP);
    EXPECT_TRUE(same_cursor(&b, &b));

    EXPECT_FALSE(same_cursor(&a, &b));
    b.type = CURSOR_DEFAULT;
    EXPECT_FALSE(same_cursor(&a, &b));
    b.color = COLOR_DEFAULT;
    EXPECT_TRUE(same_cursor(&a, &b));
}

static void test_term_parse_csi_params(TestContext *ctx)
{
    TermControlParams csi = {.nparams = 0};
    StringView s = STRING_VIEW("\033[901;0;55mx");
    size_t n = term_parse_csi_params(s.data, s.length, 2, &csi);
    EXPECT_EQ(n, s.length - 1);
    EXPECT_EQ(csi.nparams, 3);
    EXPECT_EQ(csi.params[0][0], 901);
    EXPECT_EQ(csi.params[1][0], 0);
    EXPECT_EQ(csi.params[2][0], 55);
    EXPECT_EQ(csi.nr_intermediate, 0);
    EXPECT_EQ(csi.final_byte, 'm');
    EXPECT_FALSE(csi.have_subparams);
    EXPECT_FALSE(csi.unhandled_bytes);

    csi = (TermControlParams){.nparams = 0};
    s = strview_from_cstring("\033[123;09;56:78:99m");
    n = term_parse_csi_params(s.data, s.length, 2, &csi);
    EXPECT_EQ(n, s.length);
    EXPECT_EQ(csi.nparams, 3);
    static_assert(ARRAYLEN(csi.nsub) >= 4);
    static_assert(ARRAYLEN(csi.nsub) == ARRAYLEN(csi.params));
    EXPECT_EQ(csi.nsub[0], 1);
    EXPECT_EQ(csi.nsub[1], 1);
    EXPECT_EQ(csi.nsub[2], 3);
    EXPECT_EQ(csi.nsub[3], 0);
    EXPECT_EQ(csi.params[0][0], 123);
    EXPECT_EQ(csi.params[1][0], 9);
    EXPECT_EQ(csi.params[2][0], 56);
    EXPECT_EQ(csi.params[2][1], 78);
    EXPECT_EQ(csi.params[2][2], 99);
    EXPECT_EQ(csi.params[3][0], 0);
    EXPECT_EQ(csi.nr_intermediate, 0);
    EXPECT_EQ(csi.final_byte, 'm');
    EXPECT_TRUE(csi.have_subparams);
    EXPECT_FALSE(csi.unhandled_bytes);

    csi = (TermControlParams){.nparams = 0};
    s = strview_from_cstring("\033[1:2:3;44:55;;6~");
    n = term_parse_csi_params(s.data, s.length, 2, &csi);
    EXPECT_EQ(n, s.length);
    EXPECT_EQ(csi.nparams, 4);
    EXPECT_EQ(csi.nsub[0], 3);
    EXPECT_EQ(csi.nsub[1], 2);
    EXPECT_EQ(csi.nsub[2], 1);
    EXPECT_EQ(csi.params[0][0], 1);
    EXPECT_EQ(csi.params[0][1], 2);
    EXPECT_EQ(csi.params[0][2], 3);
    EXPECT_EQ(csi.params[1][0], 44);
    EXPECT_EQ(csi.params[1][1], 55);
    EXPECT_EQ(csi.params[2][0], 0);
    EXPECT_EQ(csi.params[3][0], 6);
    EXPECT_EQ(csi.nr_intermediate, 0);
    EXPECT_EQ(csi.final_byte, '~');
    EXPECT_TRUE(csi.have_subparams);
    EXPECT_FALSE(csi.unhandled_bytes);

    csi = (TermControlParams){.nparams = 0};
    s = strview_from_cstring("\033[+2p");
    n = term_parse_csi_params(s.data, s.length, 2, &csi);
    EXPECT_EQ(n, s.length);
    EXPECT_EQ(csi.nparams, 1);
    EXPECT_EQ(csi.nsub[0], 1);
    EXPECT_EQ(csi.params[0][0], 2);
    EXPECT_EQ(csi.nr_intermediate, 1);
    EXPECT_EQ(csi.intermediate[0], '+');
    EXPECT_EQ(csi.final_byte, 'p');
    EXPECT_FALSE(csi.have_subparams);
    EXPECT_FALSE(csi.unhandled_bytes);

    csi = (TermControlParams){.nparams = 0};
    s = strview_from_cstring("\033[?47;1$y");
    n = term_parse_csi_params(s.data, s.length, 2, &csi);
    EXPECT_EQ(n, s.length);
    EXPECT_EQ(csi.nparams, 2);
    EXPECT_EQ(csi.params[0][0], 47);
    EXPECT_EQ(csi.params[1][0], 1);
    EXPECT_EQ(csi.nr_intermediate, 1);
    EXPECT_EQ(csi.intermediate[0], '$');
    EXPECT_FALSE(csi.have_subparams);
    // Note: question mark param prefixes are handled in parse_csi() instead
    // of term_parse_csi_params(), so the latter reports it as an unhandled byte
    EXPECT_TRUE(csi.unhandled_bytes);
}

static void test_term_parse_sequence(TestContext *ctx)
{
    EXPECT_PARSE_SEQ("", 0);
    EXPECT_PARSE_SEQN("x", 0, 0);
    EXPECT_PARSE_SEQN("\033q", 0, 0);
    EXPECT_PARSE_SEQ("\033\\", KEY_IGNORE);

    EXPECT_PARSE_SEQ("\033[Z", MOD_SHIFT | KEY_TAB);
    EXPECT_PARSE_SEQ("\033[1;2A", MOD_SHIFT | KEY_UP);
    EXPECT_PARSE_SEQ("\033[1;2B", MOD_SHIFT | KEY_DOWN);
    EXPECT_PARSE_SEQ("\033[1;2C", MOD_SHIFT | KEY_RIGHT);
    EXPECT_PARSE_SEQ("\033[1;2D", MOD_SHIFT | KEY_LEFT);
    EXPECT_PARSE_SEQ("\033[1;2E", MOD_SHIFT | KEY_BEGIN);
    EXPECT_PARSE_SEQ("\033[1;2F", MOD_SHIFT | KEY_END);
    EXPECT_PARSE_SEQ("\033[1;2H", MOD_SHIFT | KEY_HOME);
    EXPECT_PARSE_SEQ("\033[1;8H", MOD_SHIFT | MOD_META | MOD_CTRL | KEY_HOME);
    EXPECT_PARSE_SEQN("\033[1;8H~", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_HOME);
    EXPECT_PARSE_SEQN("\033[1;8H~_", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_HOME);
    EXPECT_PARSE_SEQN("\033", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQN("\033[", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQN("\033]", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQN("\033[1", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQN("\033[9", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQN("\033[1;", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQN("\033[1[", 4, KEY_IGNORE);
    EXPECT_PARSE_SEQN("\033[1;2", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQN("\033[1;8", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQN("\033[1;9", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQ("\033[1;_", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[1;8Z", KEY_IGNORE);
    EXPECT_PARSE_SEQN("\033O", TPARSE_PARTIAL_MATCH, 0);
    EXPECT_PARSE_SEQN("\033[\033", 2, KEY_IGNORE);
    EXPECT_PARSE_SEQN("\033[\030", 3, KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[A", KEY_UP);
    EXPECT_PARSE_SEQ("\033[B", KEY_DOWN);
    EXPECT_PARSE_SEQ("\033[C", KEY_RIGHT);
    EXPECT_PARSE_SEQ("\033[D", KEY_LEFT);
    EXPECT_PARSE_SEQ("\033[E", KEY_BEGIN);
    EXPECT_PARSE_SEQ("\033[F", KEY_END);
    EXPECT_PARSE_SEQ("\033[H", KEY_HOME);
    EXPECT_PARSE_SEQ("\033[L", KEY_INSERT);
    EXPECT_PARSE_SEQ("\033[P", KEY_F1);
    EXPECT_PARSE_SEQ("\033[Q", KEY_F2);
    EXPECT_PARSE_SEQ("\033[R", KEY_F3);
    EXPECT_PARSE_SEQ("\033[S", KEY_F4);
    EXPECT_PARSE_SEQ("\033O ", KEY_SPACE);
    EXPECT_PARSE_SEQ("\033OA", KEY_UP);
    EXPECT_PARSE_SEQ("\033OB", KEY_DOWN);
    EXPECT_PARSE_SEQ("\033OC", KEY_RIGHT);
    EXPECT_PARSE_SEQ("\033OD", KEY_LEFT);
    EXPECT_PARSE_SEQ("\033OE", KEY_BEGIN);
    EXPECT_PARSE_SEQ("\033OF", KEY_END);
    EXPECT_PARSE_SEQ("\033OH", KEY_HOME);
    EXPECT_PARSE_SEQ("\033OI", KEY_TAB);
    EXPECT_PARSE_SEQ("\033OJ", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033OM", KEY_ENTER);
    EXPECT_PARSE_SEQ("\033OP", KEY_F1);
    EXPECT_PARSE_SEQ("\033OQ", KEY_F2);
    EXPECT_PARSE_SEQ("\033OR", KEY_F3);
    EXPECT_PARSE_SEQ("\033OS", KEY_F4);
    EXPECT_PARSE_SEQ("\033OT", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033OX", '=');
    EXPECT_PARSE_SEQ("\033Oi", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033Oj", '*');
    EXPECT_PARSE_SEQ("\033Ok", '+');
    EXPECT_PARSE_SEQ("\033Ol", ',');
    EXPECT_PARSE_SEQ("\033Om", '-');
    EXPECT_PARSE_SEQ("\033On", '.');
    EXPECT_PARSE_SEQ("\033Oo", '/');
    EXPECT_PARSE_SEQ("\033Op", '0');
    EXPECT_PARSE_SEQ("\033Oq", '1');
    EXPECT_PARSE_SEQ("\033Or", '2');
    EXPECT_PARSE_SEQ("\033Os", '3');
    EXPECT_PARSE_SEQ("\033Ot", '4');
    EXPECT_PARSE_SEQ("\033Ou", '5');
    EXPECT_PARSE_SEQ("\033Ov", '6');
    EXPECT_PARSE_SEQ("\033Ow", '7');
    EXPECT_PARSE_SEQ("\033Ox", '8');
    EXPECT_PARSE_SEQ("\033Oy", '9');
    EXPECT_PARSE_SEQ("\033Oz", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033O~", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[1~", KEY_HOME);
    EXPECT_PARSE_SEQ("\033[2~", KEY_INSERT);
    EXPECT_PARSE_SEQ("\033[3~", KEY_DELETE);
    EXPECT_PARSE_SEQ("\033[4~", KEY_END);
    EXPECT_PARSE_SEQ("\033[5~", KEY_PAGE_UP);
    EXPECT_PARSE_SEQ("\033[6~", KEY_PAGE_DOWN);
    EXPECT_PARSE_SEQ("\033[7~", KEY_HOME);
    EXPECT_PARSE_SEQ("\033[8~", KEY_END);
    EXPECT_PARSE_SEQ("\033[10~", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[11~", KEY_F1);
    EXPECT_PARSE_SEQ("\033[12~", KEY_F2);
    EXPECT_PARSE_SEQ("\033[13~", KEY_F3);
    EXPECT_PARSE_SEQ("\033[14~", KEY_F4);
    EXPECT_PARSE_SEQ("\033[15~", KEY_F5);
    EXPECT_PARSE_SEQ("\033[16~", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[17~", KEY_F6);
    EXPECT_PARSE_SEQ("\033[18~", KEY_F7);
    EXPECT_PARSE_SEQ("\033[19~", KEY_F8);
    EXPECT_PARSE_SEQ("\033[20~", KEY_F9);
    EXPECT_PARSE_SEQ("\033[21~", KEY_F10);
    EXPECT_PARSE_SEQ("\033[22~", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[23~", KEY_F11);
    EXPECT_PARSE_SEQ("\033[24~", KEY_F12);
    EXPECT_PARSE_SEQ("\033[25~", KEY_F13);
    EXPECT_PARSE_SEQ("\033[26~", KEY_F14);
    EXPECT_PARSE_SEQ("\033[27~", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[28~", KEY_F15);
    EXPECT_PARSE_SEQ("\033[29~", KEY_F16);
    EXPECT_PARSE_SEQ("\033[30~", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[31~", KEY_F17);
    EXPECT_PARSE_SEQ("\033[34~", KEY_F20);
    EXPECT_PARSE_SEQ("\033[35~", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[6;3~", MOD_META | KEY_PAGE_DOWN);
    EXPECT_PARSE_SEQ("\033[6;5~", MOD_CTRL | KEY_PAGE_DOWN);
    EXPECT_PARSE_SEQ("\033[6;8~", MOD_SHIFT | MOD_META | MOD_CTRL | KEY_PAGE_DOWN);

    // xterm + `modifyOtherKeys` option
    EXPECT_PARSE_SEQ("\033[27;5;9~", MOD_CTRL | KEY_TAB);
    EXPECT_PARSE_SEQ("\033[27;5;27~", MOD_CTRL | KEY_ESCAPE);
    EXPECT_PARSE_SEQ("\033[27;8;27~", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_ESCAPE);
    EXPECT_PARSE_SEQ("\033[27;1;27~", KEY_ESCAPE); // Tested in principle only; xterm never sends this
    EXPECT_PARSE_SEQ("\033[27;5;13~", MOD_CTRL | KEY_ENTER);
    EXPECT_PARSE_SEQ("\033[27;6;13~", MOD_CTRL | MOD_SHIFT | KEY_ENTER);
    EXPECT_PARSE_SEQ("\033[27;2;127~", MOD_SHIFT | KEY_BACKSPACE);
    EXPECT_PARSE_SEQ("\033[27;6;127~", MOD_CTRL | MOD_SHIFT | KEY_BACKSPACE);
    EXPECT_PARSE_SEQ("\033[27;8;127~", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_BACKSPACE);
    EXPECT_PARSE_SEQ("\033[27;2;8~", MOD_SHIFT | KEY_BACKSPACE);
    EXPECT_PARSE_SEQ("\033[27;6;8~", MOD_CTRL | MOD_SHIFT | KEY_BACKSPACE);
    EXPECT_PARSE_SEQ("\033[27;8;8~", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_BACKSPACE);
    EXPECT_PARSE_SEQ("\033[27;6;82~", MOD_CTRL | MOD_SHIFT | 'r');
    EXPECT_PARSE_SEQ("\033[27;5;114~", MOD_CTRL | 'r');
    EXPECT_PARSE_SEQ("\033[27;3;82~", MOD_META | MOD_SHIFT | 'r'); // Sent by tmux 3.5-76-gbbc3cc55 (xterm sends "\033[27;4;82~")
    EXPECT_PARSE_SEQ("\033[27;3;114~", MOD_META | 'r');
    // EXPECT_PARSE_SEQ("\033[27;4;62~", MOD_META | '>');
    EXPECT_PARSE_SEQ("\033[27;5;46~", MOD_CTRL | '.');
    EXPECT_PARSE_SEQ("\033[27;3;1114111~", MOD_META | UNICODE_MAX_VALID_CODEPOINT);
    EXPECT_PARSE_SEQ("\033[27;3;1114112~", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[27;999999999999999999999;123~", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[27;123;99999999999999999~", KEY_IGNORE);

    // www.leonerd.org.uk/hacks/fixterms/
    EXPECT_PARSE_SEQ("\033[13;3u", MOD_META | KEY_ENTER);
    EXPECT_PARSE_SEQ("\033[9;5u", MOD_CTRL | KEY_TAB);
    EXPECT_PARSE_SEQ("\033[65;3u", MOD_META | 'A');
    EXPECT_PARSE_SEQ("\033[108;5u", MOD_CTRL | 'l');
    EXPECT_PARSE_SEQ("\033[127765;3u", MOD_META | 127765ul);
    EXPECT_PARSE_SEQ("\033[1114111;3u", MOD_META | UNICODE_MAX_VALID_CODEPOINT);
    EXPECT_PARSE_SEQ("\033[1114112;3u", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[11141110;3u", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[11141111;3u", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[2147483647;3u", KEY_IGNORE); // INT32_MAX
    EXPECT_PARSE_SEQ("\033[2147483648;3u", KEY_IGNORE); // INT32_MAX + 1
    EXPECT_PARSE_SEQ("\033[4294967295;3u", KEY_IGNORE); // UINT32_MAX
    EXPECT_PARSE_SEQ("\033[4294967296;3u", KEY_IGNORE); // UINT32_MAX + 1
    EXPECT_PARSE_SEQ("\033[-1;3u", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[-2;3u", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[ 2;3u", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[<?>2;3u", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[ !//.$2;3u", KEY_IGNORE);

    // https://sw.kovidgoyal.net/kitty/keyboard-protocol
    EXPECT_PARSE_SEQ("\033[27u", KEY_ESCAPE);
    EXPECT_PARSE_SEQ("\033[27;5u", MOD_CTRL | KEY_ESCAPE);
    EXPECT_PARSE_SEQ("\033[27;8u", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_ESCAPE);
    EXPECT_PARSE_SEQ("\033[57359u", KEY_SCROLL_LOCK);
    EXPECT_PARSE_SEQ("\033[57360u", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[57361u", KEY_PRINT_SCREEN);
    EXPECT_PARSE_SEQ("\033[57362u", KEY_PAUSE);
    EXPECT_PARSE_SEQ("\033[57363u", KEY_MENU);
    EXPECT_PARSE_SEQ("\033[57376u", KEY_F13);
    EXPECT_PARSE_SEQ("\033[57377u", KEY_F14);
    EXPECT_PARSE_SEQ("\033[57378u", KEY_F15);
    EXPECT_PARSE_SEQ("\033[57379u", KEY_F16);
    EXPECT_PARSE_SEQ("\033[57380u", KEY_F17);
    EXPECT_PARSE_SEQ("\033[57381u", KEY_F18);
    EXPECT_PARSE_SEQ("\033[57382u", KEY_F19);
    EXPECT_PARSE_SEQ("\033[57383u", KEY_F20);
    EXPECT_PARSE_SEQ("\033[57399u", '0');
    EXPECT_PARSE_SEQ("\033[57400u", '1');
    EXPECT_PARSE_SEQ("\033[57401u", '2');
    EXPECT_PARSE_SEQ("\033[57402u", '3');
    EXPECT_PARSE_SEQ("\033[57403u", '4');
    EXPECT_PARSE_SEQ("\033[57404u", '5');
    EXPECT_PARSE_SEQ("\033[57405u", '6');
    EXPECT_PARSE_SEQ("\033[57406u", '7');
    EXPECT_PARSE_SEQ("\033[57407u", '8');
    EXPECT_PARSE_SEQ("\033[57408u", '9');
    EXPECT_PARSE_SEQ("\033[57409u", '.');
    EXPECT_PARSE_SEQ("\033[57410u", '/');
    EXPECT_PARSE_SEQ("\033[57411u", '*');
    EXPECT_PARSE_SEQ("\033[57412u", '-');
    EXPECT_PARSE_SEQ("\033[57413u", '+');
    EXPECT_PARSE_SEQ("\033[57414u", KEY_ENTER);
    EXPECT_PARSE_SEQ("\033[57415u", '=');
    EXPECT_PARSE_SEQ("\033[57417u", KEY_LEFT);
    EXPECT_PARSE_SEQ("\033[57418u", KEY_RIGHT);
    EXPECT_PARSE_SEQ("\033[57419u", KEY_UP);
    EXPECT_PARSE_SEQ("\033[57420u", KEY_DOWN);
    EXPECT_PARSE_SEQ("\033[57421u", KEY_PAGE_UP);
    EXPECT_PARSE_SEQ("\033[57422u", KEY_PAGE_DOWN);
    EXPECT_PARSE_SEQ("\033[57423u", KEY_HOME);
    EXPECT_PARSE_SEQ("\033[57424u", KEY_END);
    EXPECT_PARSE_SEQ("\033[57425u", KEY_INSERT);
    EXPECT_PARSE_SEQ("\033[57426u", KEY_DELETE);
    EXPECT_PARSE_SEQ("\033[57427u", KEY_BEGIN);
    EXPECT_PARSE_SEQ("\033[57361;2u", MOD_SHIFT | KEY_PRINT_SCREEN);
    EXPECT_PARSE_SEQ("\033[3615:3620:97;6u", MOD_CTRL | MOD_SHIFT | 'a');
    EXPECT_PARSE_SEQ("\033[97;5u", MOD_CTRL | 'a');
    EXPECT_PARSE_SEQ("\033[97;5:1u", MOD_CTRL | 'a');
    EXPECT_PARSE_SEQ("\033[97;5:2u", MOD_CTRL | 'a');
    EXPECT_PARSE_SEQ("\033[97;5:3u", KEY_IGNORE);
    // TODO: EXPECT_PARSE_SEQ("\033[97;5:u", MOD_CTRL | 'a');
    EXPECT_PARSE_SEQ("\033[104;5u", MOD_CTRL | 'h');
    EXPECT_PARSE_SEQ("\033[104;9u", MOD_SUPER | 'h');
    EXPECT_PARSE_SEQ("\033[104;17u", MOD_HYPER | 'h');
    EXPECT_PARSE_SEQ("\033[104;10u", MOD_SUPER | MOD_SHIFT | 'h');
    EXPECT_PARSE_SEQ("\033[116;69u", MOD_CTRL | 't'); // Ignored Capslock in modifiers
    EXPECT_PARSE_SEQ("\033[116;133u", MOD_CTRL | 't'); // Ignored Numlock in modifiers
    EXPECT_PARSE_SEQ("\033[116;256u", MOD_MASK | 't'); // Ignored Capslock and Numlock
    EXPECT_PARSE_SEQ("\033[116;257u", KEY_IGNORE); // Unknown bit in modifiers

    // Excess params
    EXPECT_PARSE_SEQ("\033[1;2;3;4;5;6;7;8;9m", KEY_IGNORE);

    // DA1 replies
    EXPECT_PARSE_SEQ("\033[?64;15;22c", TFLAG(TFLAG_QUERY_L2 | TFLAG_QUERY_L3 | TFLAG_8_COLOR));
    EXPECT_PARSE_SEQ("\033[?1;2c", TFLAG(TFLAG_QUERY_L2));
    EXPECT_PARSE_SEQ("\033[?90c", KEY_IGNORE);

    // DA2 replies
    EXPECT_PARSE_SEQ("\033[>0;136;0c", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[>84;0;0c", TFLAG(TFLAG_QUERY_L3)); // tmux
    EXPECT_PARSE_SEQ("\033[>1;11801;0c", TFLAG(TFLAG_QUERY_L3)); // foot

    // DA3 replies
    EXPECT_PARSE_SEQN("\033P!|464f4f54\033\\", 12, TFLAG(TFLAG_QUERY_L3)); // foot
    EXPECT_PARSE_SEQN("\033P!|464f4f54\033", 12, TFLAG(TFLAG_QUERY_L3));
    EXPECT_PARSE_SEQN("\033P!|464f4f54", TPARSE_PARTIAL_MATCH, 0);

    // XTVERSION replies
    const TermFeatureFlags tmuxflags = TFLAG (
        TFLAG_NO_QUERY_L3 | TFLAG_ECMA48_REPEAT | TFLAG_MODIFY_OTHER_KEYS
        | TFLAG_BS_CTRL_BACKSPACE
    );
    EXPECT_PARSE_SEQN("\033P>|tmux 3.2\033\\", 12, tmuxflags);
    EXPECT_PARSE_SEQN("\033P>|tmux 3.2a\033\\", 13, tmuxflags);
    EXPECT_PARSE_SEQN("\033P>|tmux 3.2-rc2\033\\", 16, tmuxflags);
    EXPECT_PARSE_SEQN("\033P>|tmux next-3.3\033\\", 17, tmuxflags);
    EXPECT_PARSE_SEQN("\033P>|xyz\033\\", 7, TFLAG(TFLAG_QUERY_L3));

    // XTMODKEYS (modifyOtherKeys mode query) replies
    EXPECT_PARSE_SEQ("\033[>4;0m", TFLAG(TFLAG_MODIFY_OTHER_KEYS));
    EXPECT_PARSE_SEQ("\033[>4;1m", TFLAG(TFLAG_MODIFY_OTHER_KEYS));
    EXPECT_PARSE_SEQ("\033[>4;2m", TFLAG(TFLAG_MODIFY_OTHER_KEYS));
    EXPECT_PARSE_SEQ("\033[>4;3m", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[>4;m", TFLAG(TFLAG_MODIFY_OTHER_KEYS));
    EXPECT_PARSE_SEQ("\033[>4m", TFLAG(TFLAG_MODIFY_OTHER_KEYS));
    EXPECT_PARSE_SEQ("\033[>4;2;0m", KEY_IGNORE);

    // XTWINOPS replies
    EXPECT_PARSE_SEQN("\033]ltitle\033\\", 8, KEY_IGNORE);
    EXPECT_PARSE_SEQN("\033]Licon\033\\", 7, KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033]ltitle\a", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033]Licon\a", KEY_IGNORE);

    // DECRPM replies (to DECRQM queries)
    EXPECT_PARSE_SEQ("\033[?2026;0$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?2026;1$y", TFLAG(TFLAG_SYNC));
    EXPECT_PARSE_SEQ("\033[?2026;2$y", TFLAG(TFLAG_SYNC));
    EXPECT_PARSE_SEQ("\033[?2026;3$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?2026;4$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?2026;5$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?0;1$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?1036;2$y", TFLAG(TFLAG_META_ESC));
    EXPECT_PARSE_SEQ("\033[?1039;2$y", TFLAG(TFLAG_ALT_ESC));
    EXPECT_PARSE_SEQ("\033[?7;2$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?25;2$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?45;2$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?67;2$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?1049;2$y", KEY_IGNORE);
    EXPECT_PARSE_SEQ("\033[?2004;2$y", KEY_IGNORE);

    // Invalid, DECRPM-like sequences
    EXPECT_PARSE_SEQ("\033[?9$y", KEY_IGNORE); // Too few params
    EXPECT_PARSE_SEQ("\033[?1;2;3$y", KEY_IGNORE); // Too many params
    EXPECT_PARSE_SEQ("\033[?1;2y", KEY_IGNORE); // No '$' intermediate byte
    EXPECT_PARSE_SEQ("\033[1;2$y", KEY_IGNORE); // No '?' param prefix

    // XTGETTCAP replies
    EXPECT_PARSE_SEQN("\033P1+r626365\033\\", 11, TFLAG(TFLAG_BACK_COLOR_ERASE));
    EXPECT_PARSE_SEQN("\033P1+r74736C=1B5D323B\033\\", 20, TFLAG(TFLAG_SET_WINDOW_TITLE));
    EXPECT_PARSE_SEQN("\033P0+r\033\\", 5, KEY_IGNORE);
    EXPECT_PARSE_SEQN("\033P0+rbbccdd\033\\", 11, KEY_IGNORE);

    // DECRQSS replies
    EXPECT_PARSE_SEQN("\033P1$r0;38:2::60:70:80;48:5:255m\033\\", 31, TFLAG(TFLAG_TRUE_COLOR | TFLAG_256_COLOR)); // SGR (xterm, foot)
    EXPECT_PARSE_SEQN("\033P1$r0;38:2:60:70:80;48:5:255m\033\\", 30, TFLAG(TFLAG_TRUE_COLOR | TFLAG_256_COLOR)); // SGR (kitty)
    EXPECT_PARSE_SEQN("\033P1$r0;zm\033\\", 9, KEY_IGNORE); // Invalid SGR-like
    EXPECT_PARSE_SEQN("\033P1$r2 q\033\\", 8, KEY_IGNORE); // DECSCUSR 2 (cursor style)

    // Kitty keyboard protocol query replies
    EXPECT_PARSE_SEQ("\033[?5u", TFLAG(TFLAG_KITTY_KEYBOARD));
    EXPECT_PARSE_SEQ("\033[?1u", TFLAG(TFLAG_KITTY_KEYBOARD));
    EXPECT_PARSE_SEQ("\033[?0u", TFLAG(TFLAG_KITTY_KEYBOARD));

    // Invalid, kitty-reply-like sequences
    EXPECT_PARSE_SEQ("\033[?u", KEY_IGNORE); // Too few params (could be valid, in theory)
    EXPECT_PARSE_SEQ("\033[?1;2u", KEY_IGNORE); // Too many params
    EXPECT_PARSE_SEQ("\033[?1:2u", KEY_IGNORE); // Sub-params
    EXPECT_PARSE_SEQ("\033[?1!u", KEY_IGNORE); // Intermediate '!' byte
}

static void test_term_parse_sequence2(TestContext *ctx)
{
    static const struct {
        char key_str[3];
        char final_byte;
        KeyCode key;
    } templates[] = {
        {"1", 'A', KEY_UP},
        {"1", 'B', KEY_DOWN},
        {"1", 'C', KEY_RIGHT},
        {"1", 'D', KEY_LEFT},
        {"1", 'E', KEY_BEGIN},
        {"1", 'F', KEY_END},
        {"1", 'H', KEY_HOME},
        {"1", 'P', KEY_F1},
        {"1", 'Q', KEY_F2},
        {"1", 'R', KEY_F3}, // https://sw.kovidgoyal.net/kitty/keyboard-protocol/#:~:text=CSI%20R
        {"1", 'S', KEY_F4},
        {"2", '~', KEY_INSERT},
        {"3", '~', KEY_DELETE},
        {"5", '~', KEY_PAGE_UP},
        {"6", '~', KEY_PAGE_DOWN},
        {"11", '~', KEY_F1},
        {"12", '~', KEY_F2},
        {"13", '~', KEY_F3},
        {"14", '~', KEY_F4},
        {"15", '~', KEY_F5},
        {"17", '~', KEY_F6},
        {"18", '~', KEY_F7},
        {"19", '~', KEY_F8},
        {"20", '~', KEY_F9},
        {"21", '~', KEY_F10},
        {"23", '~', KEY_F11},
        {"24", '~', KEY_F12},
        {"25", '~', KEY_F13},
        {"26", '~', KEY_F14},
        {"28", '~', KEY_F15},
        {"29", '~', KEY_F16},
        {"31", '~', KEY_F17},
        {"32", '~', KEY_F18},
        {"33", '~', KEY_F19},
        {"34", '~', KEY_F20},
    };

    static const struct {
        char mod_str[4];
        KeyCode mask;
    } modifiers[] = {
        {"0", KEY_IGNORE},
        {"1", 0},
        {"2", MOD_SHIFT},
        {"3", MOD_META},
        {"4", MOD_SHIFT | MOD_META},
        {"5", MOD_CTRL},
        {"6", MOD_SHIFT | MOD_CTRL},
        {"7", MOD_META | MOD_CTRL},
        {"8", MOD_SHIFT | MOD_META | MOD_CTRL},
        {"9", MOD_SUPER},
        {"10", MOD_SUPER | MOD_SHIFT},
        {"11", MOD_SUPER | MOD_META},
        {"12", MOD_SUPER | MOD_META | MOD_SHIFT},
        {"13", MOD_SUPER | MOD_CTRL},
        {"14", MOD_SUPER | MOD_CTRL | MOD_SHIFT},
        {"15", MOD_SUPER | MOD_META | MOD_CTRL},
        {"16", MOD_SUPER | MOD_META | MOD_CTRL | MOD_SHIFT},
        {"17", MOD_HYPER},
        {"18", MOD_HYPER | MOD_SHIFT},
        {"255", MOD_HYPER | MOD_SUPER | MOD_META | MOD_CTRL},
        {"256", MOD_MASK},
        {"257", KEY_IGNORE},
        {"400", KEY_IGNORE},
    };

    static_assert(KEY_IGNORE != 0);
    static_assert((KEY_IGNORE & MOD_MASK) == 0);

    FOR_EACH_I(i, templates) {
        FOR_EACH_I(j, modifiers) {
            char seq[16];
            size_t seq_length = xsnprintf (
                seq,
                sizeof seq,
                "\033[%s;%s%c",
                templates[i].key_str,
                modifiers[j].mod_str,
                templates[i].final_byte
            );
            KeyCode key = 24;
            ssize_t parsed_length = term_parse_sequence(seq, seq_length, &key);
            KeyCode mods = modifiers[j].mask;
            KeyCode expected_key = mods | (mods == KEY_IGNORE ? 0 : templates[i].key);
            IEXPECT_EQ(parsed_length, seq_length);
            IEXPECT_KEYCODE_EQ(key, expected_key, seq, seq_length);
            // Truncated
            key = 25;
            for (size_t n = seq_length - 1; n != 0; n--) {
                parsed_length = term_parse_sequence(seq, n, &key);
                EXPECT_EQ(parsed_length, TPARSE_PARTIAL_MATCH);
                EXPECT_EQ(key, 25);
            }
            // Overlength
            key = 26;
            seq[seq_length++] = '~';
            parsed_length = term_parse_sequence(seq, seq_length, &key);
            IEXPECT_EQ(parsed_length, seq_length - 1);
            IEXPECT_KEYCODE_EQ(key, expected_key, seq, seq_length);
        }
    }
}

static void test_rxvt_parse_key(TestContext *ctx)
{
    static const struct {
        char escape_sequence[8];
        KeyCode key;
    } templates[] = {
        {"\033\033[2_", KEY_INSERT},
        {"\033\033[3_", KEY_DELETE},
        {"\033\033[5_", KEY_PAGE_UP},
        {"\033\033[6_", KEY_PAGE_DOWN},
        {"\033\033[7_", KEY_HOME},
        {"\033\033[8_", KEY_END},
    };

    static const struct {
        char ch;
        KeyCode mask;
    } modifiers[] = {
        {'~', 0},
        {'^', MOD_CTRL},
        {'$', MOD_SHIFT},
        {'@', MOD_SHIFT | MOD_CTRL},
    };

    FOR_EACH_I(i, templates) {
        FOR_EACH_I(j, modifiers) {
            char seq[8];
            memcpy(seq, templates[i].escape_sequence, 8);
            ASSERT_EQ(seq[7], '\0');
            char *underscore = strchr(seq, '_');
            ASSERT_NONNULL(underscore);
            *underscore = modifiers[j].ch;
            size_t seq_length = strlen(seq);
            KeyCode key;

            // Double ESC
            ssize_t parsed_length = rxvt_parse_key(seq, seq_length, &key);
            EXPECT_EQ(parsed_length, seq_length);
            EXPECT_EQ(key, modifiers[j].mask | templates[i].key | MOD_META);

            // Single ESC
            parsed_length = rxvt_parse_key(seq + 1, seq_length - 1, &key);
            EXPECT_EQ(parsed_length, seq_length - 1);
            EXPECT_EQ(key, modifiers[j].mask | templates[i].key);

            // Truncated (double ESC)
            for (size_t n = seq_length - 1; n != 0; n--) {
                key = 25;
                parsed_length = rxvt_parse_key(seq, n, &key);
                EXPECT_EQ(parsed_length, TPARSE_PARTIAL_MATCH);
                EXPECT_EQ(key, 25);
            }

            // Truncated (single ESC)
            for (size_t n = seq_length - 2; n != 0; n--) {
                key = 25;
                parsed_length = rxvt_parse_key(seq + 1, n, &key);
                EXPECT_EQ(parsed_length, TPARSE_PARTIAL_MATCH);
                EXPECT_EQ(key, 25);
            }
        }
    }

    static const struct {
        char NONSTRING seq[6];
        uint8_t seq_length;
        int8_t expected_length;
        KeyCode expected_key;
    } tests[] = {
        {STRN("\033Oa"), 3, MOD_CTRL | KEY_UP},
        {STRN("\033Ob"), 3, MOD_CTRL | KEY_DOWN},
        {STRN("\033Oc"), 3, MOD_CTRL | KEY_RIGHT},
        {STRN("\033Od"), 3, MOD_CTRL | KEY_LEFT},
        {STRN("\033O"), TPARSE_PARTIAL_MATCH, 0},
        {STRN("\033[a"), 3, MOD_SHIFT | KEY_UP},
        {STRN("\033[b"), 3, MOD_SHIFT | KEY_DOWN},
        {STRN("\033[c"), 3, MOD_SHIFT | KEY_RIGHT},
        {STRN("\033[d"), 3, MOD_SHIFT | KEY_LEFT},
        {STRN("\033["), TPARSE_PARTIAL_MATCH, 0},
        {STRN("\033[1;5A"), 6, MOD_CTRL | KEY_UP},
        {STRN("\033[1;5"), TPARSE_PARTIAL_MATCH, 0},
        {STRN("\033\033[@"), 4, KEY_IGNORE},
        {STRN("\033\033["), TPARSE_PARTIAL_MATCH, 0},
        {STRN("\033\033"), TPARSE_PARTIAL_MATCH, 0},
        {STRN("\033"), TPARSE_PARTIAL_MATCH, 0},
    };

    FOR_EACH_I(i, tests) {
        const char *seq = tests[i].seq;
        const size_t seq_length = tests[i].seq_length;
        KeyCode key = 0x18;
        ssize_t parsed_length = rxvt_parse_key(seq, seq_length, &key);
        KeyCode expected_key = (parsed_length > 0) ? tests[i].expected_key : 0x18;
        IEXPECT_EQ(parsed_length, tests[i].expected_length);
        IEXPECT_KEYCODE_EQ(key, expected_key, seq, seq_length);
    }
}

static void test_linux_parse_key(TestContext *ctx)
{
    static const struct {
        char NONSTRING seq[6];
        uint8_t seq_length;
        int8_t expected_length;
        KeyCode expected_key;
    } tests[] = {
        {STRN("\033[1;5A"), 6, MOD_CTRL | KEY_UP},
        {STRN("\033[[A"), 4, KEY_F1},
        {STRN("\033[[B"), 4, KEY_F2},
        {STRN("\033[[C"), 4, KEY_F3},
        {STRN("\033[[D"), 4, KEY_F4},
        {STRN("\033[[E"), 4, KEY_F5},
        {STRN("\033[[F"), 0, 0},
        {STRN("\033[["), TPARSE_PARTIAL_MATCH, 0},
        {STRN("\033["), TPARSE_PARTIAL_MATCH, 0},
        {STRN("\033"), TPARSE_PARTIAL_MATCH, 0},
    };

    FOR_EACH_I(i, tests) {
        const char *seq = tests[i].seq;
        const size_t seq_length = tests[i].seq_length;
        KeyCode key = 0x18;
        ssize_t parsed_length = linux_parse_key(seq, seq_length, &key);
        KeyCode expected_key = (parsed_length > 0) ? tests[i].expected_key : 0x18;
        IEXPECT_EQ(parsed_length, tests[i].expected_length);
        IEXPECT_KEYCODE_EQ(key, expected_key, seq, seq_length);
    }
}

static void test_keycode_to_string(TestContext *ctx)
{
    static const struct {
        const char *str;
        KeyCode key;
    } tests[] = {
        {"a", 'a'},
        {"Z", 'Z'},
        {"0", '0'},
        {"{", '{'},
        {"space", KEY_SPACE},
        {"enter", KEY_ENTER},
        {"tab", KEY_TAB},
        {"escape", KEY_ESCAPE},
        {"backspace", KEY_BACKSPACE},
        {"insert", KEY_INSERT},
        {"delete", KEY_DELETE},
        {"home", KEY_HOME},
        {"end", KEY_END},
        {"pgup", KEY_PAGE_UP},
        {"pgdown", KEY_PAGE_DOWN},
        {"begin", KEY_BEGIN},
        {"left", KEY_LEFT},
        {"right", KEY_RIGHT},
        {"up", KEY_UP},
        {"down", KEY_DOWN},
        {"scrlock", KEY_SCROLL_LOCK},
        {"print", KEY_PRINT_SCREEN},
        {"pause", KEY_PAUSE},
        {"menu", KEY_MENU},
        {"C-a", MOD_CTRL | 'a'},
        {"C-s", MOD_CTRL | 's'},
        {"M-a", MOD_META | 'a'},
        {"M-S-{", MOD_META | MOD_SHIFT | '{'},
        {"C-S-a", MOD_CTRL | MOD_SHIFT | 'a'},
        {"M-S-a", MOD_META | MOD_SHIFT | 'a'},
        {"F1", KEY_F1},
        {"F5", KEY_F5},
        {"F12", KEY_F12},
        {"F13", KEY_F13},
        {"F16", KEY_F16},
        {"F20", KEY_F20},
        {"M-enter", MOD_META | KEY_ENTER},
        {"M-space", MOD_META | KEY_SPACE},
        {"S-tab", MOD_SHIFT | KEY_TAB},
        {"C-M-S-F12", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_F12},
        {"C-M-F16", MOD_CTRL | MOD_META | KEY_F16},
        {"C-S-F20", MOD_CTRL | MOD_SHIFT | KEY_F20},
        {"C-M-S-up", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_UP},
        {"C-M-delete", MOD_CTRL | MOD_META | KEY_DELETE},
        {"C-home", MOD_CTRL | KEY_HOME},
        {"s-space", MOD_SUPER | ' '},
        {"H-end", MOD_HYPER | KEY_END},
#if __STDC_VERSION__ >= 201112L
        {u8"ก", 0x0E01},
        {u8"C-ก", MOD_CTRL | 0x0E01},
        {u8"M-Ф", MOD_META | 0x0424},
#endif
    };

    char buf[KEYCODE_STR_BUFSIZE];
    ASSERT_TRUE(sizeof(buf) >= sizeof("QUERY REPLY; 0x12345678"));
    EXPECT_EQ(KEYCODE_STR_BUFSIZE, next_pow2(sizeof("QUERY REPLY; 0x12345678")));

    FOR_EACH_I(i, tests) {
        const char *str = tests[i].str;
        size_t len = strlen(str);
        ASSERT_TRUE(len < sizeof(buf));
        size_t buflen = keycode_to_str(tests[i].key, buf);
        IEXPECT_STREQ(buf, str);
        IEXPECT_EQ(buflen, len);
        IEXPECT_EQ(keycode_from_str(str), tests[i].key);
    }

    EXPECT_EQ(keycode_to_str(KEYCODE_DETECTED_PASTE, buf), 19);
    EXPECT_STREQ(buf, "INVALID; 0x00110025");
    EXPECT_EQ(keycode_to_str(KEYCODE_BRACKETED_PASTE, buf), 19);
    EXPECT_STREQ(buf, "INVALID; 0x00110026");
    EXPECT_EQ(keycode_to_str(KEY_IGNORE, buf), 19);
    EXPECT_STREQ(buf, "INVALID; 0x00110027");
    EXPECT_EQ(keycode_to_str(UINT32_MAX, buf), 23);
    EXPECT_STREQ(buf, "QUERY REPLY; 0xFFFFFFFF");

    // These combos aren't round-trippable by the code above and can't end
    // up in real bindings, since the letters are normalized to lower case
    // by keycode_from_str(). We still test them nevertheless; for the sake
    // of completeness and catching unexpected changes.
    EXPECT_EQ(keycode_to_str(MOD_CTRL | 'A', buf), 3);
    EXPECT_STREQ(buf, "C-A");
    EXPECT_EQ(keycode_to_str(MOD_CTRL | MOD_SHIFT | 'A', buf), 5);
    EXPECT_STREQ(buf, "C-S-A");
    EXPECT_EQ(keycode_to_str(MOD_META | MOD_SHIFT | 'A', buf), 5);
    EXPECT_STREQ(buf, "M-S-A");
}

static void test_parse_key_string(TestContext *ctx)
{
    EXPECT_EQ(keycode_from_str("^I"), MOD_CTRL | 'i');
    EXPECT_EQ(keycode_from_str("^M"), MOD_CTRL | 'm');
    EXPECT_EQ(keycode_from_str("C-I"), MOD_CTRL | 'i');
    EXPECT_EQ(keycode_from_str("C-M"), MOD_CTRL | 'm');
    EXPECT_EQ(keycode_from_str("C-F1"), MOD_CTRL | KEY_F1);
    EXPECT_EQ(keycode_from_str("C-M-S-F20"), MOD_CTRL | MOD_META | MOD_SHIFT | KEY_F20);
    EXPECT_EQ(keycode_from_str("s-m"), MOD_SUPER | 'm');
    EXPECT_EQ(keycode_from_str("H-y"), MOD_HYPER | 'y');
    EXPECT_EQ(keycode_from_str("H-y"), MOD_HYPER | 'y');
    EXPECT_EQ(keycode_from_str("H-S-z"), MOD_HYPER | MOD_SHIFT | 'z');
    EXPECT_EQ(keycode_from_str("s-S-t"), MOD_SUPER | MOD_SHIFT | 't');
    EXPECT_EQ(keycode_from_str("S-print"), MOD_SHIFT | KEY_PRINT_SCREEN);
    EXPECT_EQ(keycode_from_str("C-S-scrlock"), MOD_CTRL | MOD_SHIFT | KEY_SCROLL_LOCK);
    EXPECT_EQ(keycode_from_str("tab"), KEY_TAB);
    EXPECT_EQ(keycode_from_str("enter"), KEY_ENTER);
    EXPECT_EQ(keycode_from_str("space"), KEY_SPACE);
    EXPECT_EQ(keycode_from_str("escape"), KEY_ESCAPE);
    EXPECT_EQ(keycode_from_str("backspace"), KEY_BACKSPACE);
    EXPECT_EQ(keycode_from_str("insert"), KEY_INSERT);
    EXPECT_EQ(keycode_from_str("delete"), KEY_DELETE);
    EXPECT_EQ(keycode_from_str("up"), KEY_UP);
    EXPECT_EQ(keycode_from_str("down"), KEY_DOWN);
    EXPECT_EQ(keycode_from_str("right"), KEY_RIGHT);
    EXPECT_EQ(keycode_from_str("left"), KEY_LEFT);
    EXPECT_EQ(keycode_from_str("begin"), KEY_BEGIN);
    EXPECT_EQ(keycode_from_str("end"), KEY_END);
    EXPECT_EQ(keycode_from_str("pgdown"), KEY_PAGE_DOWN);
    EXPECT_EQ(keycode_from_str("home"), KEY_HOME);
    EXPECT_EQ(keycode_from_str("pgup"), KEY_PAGE_UP);
    EXPECT_EQ(keycode_from_str("scrlock"), KEY_SCROLL_LOCK);
    EXPECT_EQ(keycode_from_str("print"), KEY_PRINT_SCREEN);
    EXPECT_EQ(keycode_from_str("pause"), KEY_PAUSE);
    EXPECT_EQ(keycode_from_str("menu"), KEY_MENU);
    EXPECT_EQ(keycode_from_str("f1"), KEY_F1);
    EXPECT_EQ(keycode_from_str("f2"), KEY_F2);
    EXPECT_EQ(keycode_from_str("f3"), KEY_F3);
    EXPECT_EQ(keycode_from_str("f4"), KEY_F4);
    EXPECT_EQ(keycode_from_str("f5"), KEY_F5);
    EXPECT_EQ(keycode_from_str("f6"), KEY_F6);
    EXPECT_EQ(keycode_from_str("f7"), KEY_F7);
    EXPECT_EQ(keycode_from_str("f8"), KEY_F8);
    EXPECT_EQ(keycode_from_str("f9"), KEY_F9);
    EXPECT_EQ(keycode_from_str("f10"), KEY_F10);
    EXPECT_EQ(keycode_from_str("f11"), KEY_F11);
    EXPECT_EQ(keycode_from_str("f12"), KEY_F12);
    EXPECT_EQ(keycode_from_str("f13"), KEY_F13);
    EXPECT_EQ(keycode_from_str("f14"), KEY_F14);
    EXPECT_EQ(keycode_from_str("f15"), KEY_F15);
    EXPECT_EQ(keycode_from_str("f16"), KEY_F16);
    EXPECT_EQ(keycode_from_str("f17"), KEY_F17);
    EXPECT_EQ(keycode_from_str("f18"), KEY_F18);
    EXPECT_EQ(keycode_from_str("f19"), KEY_F19);
    EXPECT_EQ(keycode_from_str("f20"), KEY_F20);

    EXPECT_EQ(keycode_from_str("C-"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("C-M-"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("paste"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("???"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("F0"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("F21"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("F01"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("\t"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("\n"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("\r"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("\x1f"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("\x7f"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("C-\t"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("C-\r"), KEY_NONE);
    EXPECT_EQ(keycode_from_str("C-\x7f"), KEY_NONE);

    // Special cases for normalization:
    EXPECT_EQ(keycode_from_str("C-A"), MOD_CTRL | 'a');
    EXPECT_EQ(keycode_from_str("C-S-A"), MOD_CTRL | MOD_SHIFT | 'a');
    EXPECT_EQ(keycode_from_str("M-A"), MOD_META | MOD_SHIFT | 'a');
    EXPECT_EQ(keycode_from_str("C-?"), MOD_CTRL | '?');
    EXPECT_EQ(keycode_from_str("C-H"), MOD_CTRL | 'h');
    EXPECT_EQ(keycode_from_str("M-C-?"), MOD_META | MOD_CTRL | '?');
    EXPECT_EQ(keycode_from_str("M-C-H"), MOD_META | MOD_CTRL | 'h');
}

static bool clear_obuf(TermOutputBuffer *obuf)
{
    if (unlikely(obuf_avail(obuf) <= 8)) {
        return false;
    }
    memset(obuf->buf, '\0', obuf->count + 8);
    obuf->count = 0;
    obuf->x = 0;
    return true;
}

static void test_term_init(TestContext *ctx)
{
    const TermFeatureFlags expected_xterm_flags =
        TFLAG_256_COLOR |
        TFLAG_16_COLOR |
        TFLAG_8_COLOR |
        TFLAG_BACK_COLOR_ERASE |
        TFLAG_SET_WINDOW_TITLE |
        TFLAG_OSC52_COPY
    ;

    Terminal term = {.obuf = TERM_OUTPUT_INIT};
    term_init(&term, "xterm-256color", NULL);
    EXPECT_EQ(term.width, 80);
    EXPECT_EQ(term.height, 24);
    EXPECT_EQ(term.ncv_attributes, 0);
    EXPECT_PTREQ(term.parse_input, term_parse_sequence);
    EXPECT_EQ(term.features, expected_xterm_flags);

    term_init(&term, "ansi", NULL);
    EXPECT_EQ(term.width, 80);
    EXPECT_EQ(term.height, 24);
    EXPECT_EQ(term.ncv_attributes, ATTR_UNDERLINE);
    EXPECT_PTREQ(term.parse_input, term_parse_sequence);
    EXPECT_EQ(term.features, TFLAG_8_COLOR | TFLAG_NCV_UNDERLINE);

    term_init(&term, "ansi-m", NULL);
    EXPECT_EQ(term.width, 80);
    EXPECT_EQ(term.height, 24);
    EXPECT_EQ(term.ncv_attributes, 0);
    EXPECT_PTREQ(term.parse_input, term_parse_sequence);
    EXPECT_EQ(term.features, 0);
}

static void test_term_put_str(TestContext *ctx)
{
    Terminal term = {
        .width = 80,
        .height = 24,
        .obuf = TERM_OUTPUT_INIT,
    };

    // Fill start of buffer with zeroes, to allow using EXPECT_STREQ() below
    TermOutputBuffer *obuf = &term.obuf;
    ASSERT_TRUE(256 <= TERM_OUTBUF_SIZE);
    memset(obuf->buf, 0, 256);

    obuf->width = 0;
    term_put_str(obuf, "this should write nothing because obuf->width == 0");
    EXPECT_EQ(obuf->count, 0);
    EXPECT_EQ(obuf->x, 0);

    term_output_reset(&term, 0, 80, 0);
    EXPECT_EQ(obuf->tab_mode, TAB_CONTROL);
    EXPECT_EQ(obuf->tab_width, 8);
    EXPECT_EQ(obuf->x, 0);
    EXPECT_EQ(obuf->width, 80);
    EXPECT_EQ(obuf->scroll_x, 0);
    EXPECT_EQ(term.width, 80);
    EXPECT_EQ(obuf->can_clear, true);

    term_put_str(obuf, "1\xF0\x9F\xA7\xB2 \t xyz \t\r \xC2\xB6");
    EXPECT_EQ(obuf->count, 20);
    EXPECT_EQ(obuf->x, 17);
    EXPECT_STREQ(obuf->buf, "1\xF0\x9F\xA7\xB2 ^I xyz ^I^M \xC2\xB6");

    EXPECT_TRUE(term_put_char(obuf, 0x10FFFF));
    EXPECT_EQ(obuf->count, 24);
    EXPECT_EQ(obuf->x, 21);
    EXPECT_STREQ(obuf->buf + 20, "<" "??" ">");
    ASSERT_TRUE(clear_obuf(obuf));
}

static void test_term_clear_eol(TestContext *ctx)
{
    Terminal term = {
        .width = 80,
        .height = 24,
        .obuf = TERM_OUTPUT_INIT,
    };

    // BCE with non-default bg
    TermOutputBuffer *obuf = &term.obuf;
    term.features = TFLAG_BACK_COLOR_ERASE;
    obuf->style = (TermStyle){.bg = COLOR_RED};
    term_output_reset(&term, 0, 80, 0);
    EXPECT_EQ(term_clear_eol(&term), TERM_CLEAR_EOL_USED_EL);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\033[K", 3);
    EXPECT_EQ(obuf->x, 80);
    ASSERT_TRUE(clear_obuf(obuf));

    // No BCE, but with REP
    term.features = TFLAG_ECMA48_REPEAT;
    obuf->style = (TermStyle){.bg = COLOR_RED};
    term_output_reset(&term, 0, 40, 0);
    EXPECT_EQ(term_clear_eol(&term), -40);
    EXPECT_MEMEQ(obuf->buf, obuf->count, " \033[39b", 6);
    EXPECT_EQ(obuf->x, 40);
    ASSERT_TRUE(clear_obuf(obuf));

    // No BCE with non-default bg
    term.features = 0;
    obuf->style = (TermStyle){.bg = COLOR_RED};
    term_output_reset(&term, 0, 80, 0);
    EXPECT_EQ(term_clear_eol(&term), 80);
    EXPECT_EQ(obuf->count, 80);
    EXPECT_EQ(obuf->x, 80);
    EXPECT_EQ(obuf->buf[0], ' ');
    EXPECT_EQ(obuf->buf[79], ' ');
    ASSERT_TRUE(clear_obuf(obuf));

    // No BCE with default bg/attrs
    term.features = 0;
    obuf->style = (TermStyle){.bg = COLOR_DEFAULT};
    term_output_reset(&term, 0, 80, 0);
    EXPECT_EQ(term_clear_eol(&term), TERM_CLEAR_EOL_USED_EL);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\033[K", 3);
    EXPECT_EQ(obuf->x, 80);
    ASSERT_TRUE(clear_obuf(obuf));

    // No BCE with ATTR_REVERSE
    term.features = 0;
    obuf->style = (TermStyle){.bg = COLOR_DEFAULT, .attr = ATTR_REVERSE};
    term_output_reset(&term, 0, 80, 0);
    EXPECT_EQ(term_clear_eol(&term), 80);
    EXPECT_EQ(obuf->count, 80);
    EXPECT_EQ(obuf->x, 80);
    ASSERT_TRUE(clear_obuf(obuf));

    // x >= scroll_x + width
    term_output_reset(&term, 0, 20, 10);
    EXPECT_EQ(obuf->width, 20);
    EXPECT_EQ(obuf->scroll_x, 10);
    obuf->x = 30;
    EXPECT_EQ(term_clear_eol(&term), 0);
    EXPECT_EQ(obuf->count, 0);
    EXPECT_EQ(obuf->x, 30);
    ASSERT_TRUE(clear_obuf(obuf));
}

static void test_term_move_cursor(TestContext *ctx)
{
    TermOutputBuffer obuf = TERM_OUTPUT_INIT;
    term_move_cursor(&obuf, 12, 5);
    EXPECT_MEMEQ(obuf.buf, obuf.count, "\033[6;13H", 7);
    EXPECT_EQ(obuf.x, 0);
    ASSERT_TRUE(clear_obuf(&obuf));

    term_move_cursor(&obuf, 0, 22);
    EXPECT_MEMEQ(obuf.buf, obuf.count, "\033[23H", 5);
    EXPECT_EQ(obuf.x, 0);
    ASSERT_TRUE(clear_obuf(&obuf));
}

static void test_term_set_bytes(TestContext *ctx)
{
    Terminal term = {
        .width = 80,
        .height = 24,
        .obuf = TERM_OUTPUT_INIT,
        .features = TFLAG_ECMA48_REPEAT,
    };

    TermOutputBuffer *obuf = &term.obuf;
    ASSERT_TRUE(clear_obuf(obuf));
    term_output_reset(&term, 0, 80, 0);

    EXPECT_EQ(term_set_bytes(&term, 'x', 40), TERM_SET_BYTES_REP);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "x\033[39b", 6);
    EXPECT_EQ(obuf->x, 40);
    ASSERT_TRUE(clear_obuf(obuf));

    const size_t repmin = ECMA48_REP_MIN - 1;
    EXPECT_EQ(repmin, 5);
    EXPECT_EQ(term_set_bytes(&term, '-', repmin), TERM_SET_BYTES_MEMSET);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "-----", repmin);
    EXPECT_EQ(obuf->x, repmin);
    ASSERT_TRUE(clear_obuf(obuf));

    EXPECT_EQ(term_set_bytes(&term, '\n', 8), TERM_SET_BYTES_MEMSET);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\n\n\n\n\n\n\n\n", 8);
    EXPECT_EQ(obuf->x, 8);
    ASSERT_TRUE(clear_obuf(obuf));
}

static void test_term_set_style(TestContext *ctx)
{
    Terminal term = {.obuf = TERM_OUTPUT_INIT};
    term_init(&term, "tmux", "truecolor");
    EXPECT_TRUE(term.features & TFLAG_TRUE_COLOR);
    EXPECT_EQ(term.ncv_attributes, 0);

    TermOutputBuffer *obuf = &term.obuf;
    ASSERT_TRUE(TERM_OUTBUF_SIZE >= 64);
    memset(obuf->buf, '?', 64);

    TermStyle style = {
        .fg = COLOR_RED,
        .bg = COLOR_YELLOW,
        .attr = ATTR_BOLD | ATTR_REVERSE,
    };

    term_set_style(&term, style);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\033[0;1;7;31;43m", 14);
    EXPECT_EQ(obuf->x, 0);
    ASSERT_TRUE(clear_obuf(obuf));

    style.attr = 0;
    style.fg = COLOR_RGB(0x12ef46);
    term_set_style(&term, style);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\033[0;38;2;18;239;70;43m", 22);
    EXPECT_EQ(obuf->x, 0);
    ASSERT_TRUE(clear_obuf(obuf));

    style.fg = 144;
    style.bg = COLOR_DEFAULT;
    term_set_style(&term, style);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\033[0;38;5;144m", 13);
    EXPECT_EQ(obuf->x, 0);
    ASSERT_TRUE(clear_obuf(obuf));

    style.fg = COLOR_GRAY;
    style.bg = COLOR_RGB(0x00b91f);
    term_set_style(&term, style);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\033[0;37;48;2;0;185;31m", 21);
    EXPECT_EQ(obuf->x, 0);
    ASSERT_TRUE(clear_obuf(obuf));

    style.fg = COLOR_WHITE;
    style.bg = COLOR_DARKGRAY;
    style.attr = ATTR_ITALIC;
    term_set_style(&term, style);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\033[0;3;97;100m", 13);
    EXPECT_EQ(obuf->x, 0);
    ASSERT_TRUE(clear_obuf(obuf));

    // Ensure longest sequence doesn't trigger assertion
    static const char longest[] =
        "\033[0;"
        "1;2;3;4;5;7;8;9;"
        "38;2;100;101;199;"
        "48;2;200;202;255m"
    ;
    const size_t n = sizeof(longest) - 1;
    ASSERT_EQ(n, 54);
    style.fg = COLOR_RGB(0x6465C7);
    style.bg = COLOR_RGB(0xC8CAFF);
    style.attr = ~0u;
    term_set_style(&term, style);
    EXPECT_MEMEQ(obuf->buf, obuf->count, longest, n);
    EXPECT_EQ(obuf->x, 0);
    ASSERT_TRUE(clear_obuf(obuf));

    style.fg = COLOR_DEFAULT;
    style.bg = COLOR_DEFAULT;
    style.attr = ATTR_REVERSE | ATTR_DIM | ATTR_UNDERLINE | ATTR_KEEP;
    term.ncv_attributes = ATTR_DIM | ATTR_STRIKETHROUGH;
    term_set_style(&term, style);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\033[0;2;4;7m", 10);
    EXPECT_EQ(obuf->x, 0);
    style.attr &= ~ATTR_KEEP;
    EXPECT_TRUE(same_style(&obuf->style, &style));
    ASSERT_TRUE(clear_obuf(obuf));

    style.fg = COLOR_BLUE;
    term_set_style(&term, style);
    EXPECT_MEMEQ(obuf->buf, obuf->count, "\033[0;4;7;34m", 11);
    EXPECT_EQ(obuf->x, 0);
    style.attr &= ~ATTR_DIM;
    EXPECT_TRUE(same_style(&obuf->style, &style));
    ASSERT_TRUE(clear_obuf(obuf));
}

static void test_term_osc52_copy(TestContext *ctx)
{
    TermOutputBuffer obuf = TERM_OUTPUT_INIT;
    EXPECT_TRUE(term_osc52_copy(&obuf, STRN("foobar"), true, true));
    EXPECT_MEMEQ(obuf.buf, obuf.count, "\033]52;pc;Zm9vYmFy\033\\", 18);
    EXPECT_EQ(obuf.x, 0);
    ASSERT_TRUE(clear_obuf(&obuf));

    EXPECT_TRUE(term_osc52_copy(&obuf, STRN("\xF0\x9F\xA5\xA3"), false, true));
    EXPECT_MEMEQ(obuf.buf, obuf.count, "\033]52;p;8J+low==\033\\", 17);
    EXPECT_EQ(obuf.x, 0);
    ASSERT_TRUE(clear_obuf(&obuf));

    EXPECT_TRUE(term_osc52_copy(&obuf, STRN(""), true, false));
    EXPECT_MEMEQ(obuf.buf, obuf.count, "\033]52;c;\033\\", 9);
    EXPECT_EQ(obuf.x, 0);
    ASSERT_TRUE(clear_obuf(&obuf));
}

static void test_term_set_cursor_style(TestContext *ctx)
{
    Terminal term = {
        .width = 80,
        .height = 24,
        .obuf = TERM_OUTPUT_INIT,
    };

    TermCursorStyle style = {
        .type = CURSOR_STEADY_BAR,
        .color = COLOR_RGB(0x22AACC),
    };

    static const char expected[] = "\033[6 q\033]12;rgb:22/aa/cc\033\\";
    size_t expected_len = sizeof(expected) - 1;
    ASSERT_EQ(expected_len, 24);

    TermOutputBuffer *obuf = &term.obuf;
    memset(obuf->buf, '@', expected_len + 16);

    term_set_cursor_style(&term, style);
    EXPECT_MEMEQ(obuf->buf, obuf->count, expected, expected_len);
    EXPECT_EQ(obuf->x, 0);
    EXPECT_EQ(obuf->cursor_style.type, style.type);
    EXPECT_EQ(obuf->cursor_style.color, style.color);
    ASSERT_TRUE(clear_obuf(obuf));
}

static void test_term_restore_cursor_style(TestContext *ctx)
{
    Terminal term = {
        .width = 80,
        .height = 24,
        .obuf = TERM_OUTPUT_INIT,
    };

    static const char expected[] = "\033[0 q\033]112\033\\";
    size_t expected_len = sizeof(expected) - 1;
    ASSERT_EQ(expected_len, 12);

    TermOutputBuffer *obuf = &term.obuf;
    memset(obuf->buf, '@', expected_len + 16);

    term_restore_cursor_style(&term);
    EXPECT_MEMEQ(obuf->buf, obuf->count, expected, expected_len);
    EXPECT_EQ(obuf->x, 0);
    ASSERT_TRUE(clear_obuf(obuf));
}

static void test_term_begin_sync_update(TestContext *ctx)
{
    Terminal term = {.obuf = TERM_OUTPUT_INIT};
    term_init(&term, "xterm-kitty", NULL);
    EXPECT_TRUE(term.features & TFLAG_SYNC);

    static const char expected[] =
        "\033[?2026h"
        "\033[?1049h"
        "\033[?25l"
        "\033[?25h"
        "\033[?1049l"
        "\033[?2026l"
    ;

    TermOutputBuffer *obuf = &term.obuf;
    memset(obuf->buf, '.', 128);

    term_begin_sync_update(&term);
    term_use_alt_screen_buffer(&term);
    term_hide_cursor(&term);
    term_show_cursor(&term);
    term_use_normal_screen_buffer(&term);
    term_end_sync_update(&term);

    EXPECT_MEMEQ(obuf->buf, obuf->count, expected, sizeof(expected) - 1);
    EXPECT_EQ(obuf->x, 0);
}

static void test_term_put_level_1_queries(TestContext *ctx)
{
    enum {
        // Short aliases for TermFeatureFlags:
        BCE = TFLAG_BACK_COLOR_ERASE,
        TITLE = TFLAG_SET_WINDOW_TITLE,
        OSC52 = TFLAG_OSC52_COPY,
        C8 = TFLAG_8_COLOR,
        C16 = TFLAG_16_COLOR | C8,
        C256 = TFLAG_256_COLOR | C16,
    };

    Terminal term = {.obuf = TERM_OUTPUT_INIT};
    TermOutputBuffer *obuf = &term.obuf;
    term_init(&term, "xterm-256color", NULL);
    EXPECT_UINT_EQ(term.features, (C256 | BCE | TITLE | OSC52));
    EXPECT_EQ(obuf->count, 0);

    // Basic level 1 queries
    static const char level1[] = "\033[c";
    term_put_initial_queries(&term, 1);
    EXPECT_MEMEQ(obuf->buf, obuf->count, level1, sizeof(level1) - 1);

    // All queries (forced by emit_all=true)
    static const char full[] =
        // term_put_initial_queries()
        "\033[c"
        // term_put_level_2_queries()
        "\033[>0q"
        "\033[>c"
        "\033[?u"
        "\033[?1036$p"
        "\033[?1039$p"
        "\033[?2026$p"
        // term_put_level_2_queries() debug
        "\033[?4m"
        "\033[?7$p"
        "\033[?25$p"
        "\033[?45$p"
        "\033[?67$p"
        "\033[?1049$p"
        "\033[?2004$p"
        "\033[18t"
        // term_put_level_3_queries()
        "\033[0;38;2;60;70;80;48;5;255m"
        "\033P$qm\033\\"
        "\033[0m"
        "\033P+q626365\033\\"
        "\033P+q726570\033\\"
        "\033P+q74736C\033\\"
        "\033P+q4D73\033\\"
        // term_put_level_3_queries() debug
        "\033P$q q\033\\"
    ;

    obuf->count = 0;
    term_put_initial_queries(&term, 6);
    EXPECT_MEMEQ(obuf->buf, obuf->count, full, sizeof(full) - 1);

    EXPECT_EQ(obuf->scroll_x, 0);
    EXPECT_EQ(obuf->x, 0);
}

static void test_update_term_title(TestContext *ctx)
{
    static const char prefix[] = "\033]2;";
    static const char suffix[] = " - dte\033\\";
    size_t plen = sizeof(prefix) - 1;
    size_t slen = sizeof(suffix) - 1;
    TermOutputBuffer obuf = TERM_OUTPUT_INIT;

    static const char expected1[] = "\033]2;example filename - dte\033\\";
    update_term_title(&obuf, "example filename", false);
    EXPECT_MEMEQ(obuf.buf, obuf.count, expected1, sizeof(expected1) - 1);
    EXPECT_EQ(obuf.x, 0);
    obuf.count = 0;

    // Control char escaping
    static const char expected2[] = "\033]2;x^I^H^[y - dte\033\\";
    update_term_title(&obuf, "x\t\b\033y", false);
    EXPECT_MEMEQ(obuf.buf, obuf.count, expected2, sizeof(expected2) - 1);
    EXPECT_EQ(obuf.x, 0);
    obuf.count = 0;

    // Very long filename, for edge case coverage
    char filename[TERM_OUTBUF_SIZE];
    memset(filename, 'a', sizeof(filename));
    filename[sizeof(filename) - 1] = '\0';
    update_term_title(&obuf, filename, false);

    size_t tlen = obuf.count - (plen + slen);
    EXPECT_EQ(sizeof(filename), 8192);
    EXPECT_EQ(obuf.count, 8188);
    EXPECT_EQ(tlen, 8176);
    EXPECT_MEMEQ(obuf.buf, plen, prefix, plen);
    EXPECT_MEMEQ(obuf.buf + plen, tlen, filename, tlen);
    EXPECT_MEMEQ(obuf.buf + plen + tlen, slen, suffix, slen);
}

static void test_is_newly_detected_feature(TestContext *ctx)
{
    EXPECT_FALSE(is_newly_detected_feature(1, 1, 1));
    EXPECT_FALSE(is_newly_detected_feature(1, 2, 1));
    EXPECT_FALSE(is_newly_detected_feature(3, 1, 1));
    EXPECT_FALSE(is_newly_detected_feature(1, 3, 1));
    EXPECT_FALSE(is_newly_detected_feature(0, 6, 1));
    EXPECT_TRUE(is_newly_detected_feature(0, 1, 1));
    EXPECT_TRUE(is_newly_detected_feature(2, 1, 1));
    EXPECT_TRUE(is_newly_detected_feature(3, 4, 4));
}

static const TestEntry tests[] = {
    TEST(test_parse_rgb),
    TEST(test_parse_term_style),
    TEST(test_color_to_nearest),
    TEST(test_color_to_str),
    TEST(test_term_style_to_string),
    TEST(test_cursor_mode_from_str),
    TEST(test_cursor_type_from_str),
    TEST(test_cursor_color_from_str),
    TEST(test_cursor_color_to_str),
    TEST(test_same_cursor),
    TEST(test_term_parse_csi_params),
    TEST(test_term_parse_sequence),
    TEST(test_term_parse_sequence2),
    TEST(test_rxvt_parse_key),
    TEST(test_linux_parse_key),
    TEST(test_keycode_to_string),
    TEST(test_parse_key_string),
    TEST(test_term_init),
    TEST(test_term_put_str),
    TEST(test_term_clear_eol),
    TEST(test_term_move_cursor),
    TEST(test_term_set_bytes),
    TEST(test_term_set_style),
    TEST(test_term_osc52_copy),
    TEST(test_term_set_cursor_style),
    TEST(test_term_restore_cursor_style),
    TEST(test_term_begin_sync_update),
    TEST(test_term_put_level_1_queries),
    TEST(test_update_term_title),
    TEST(test_is_newly_detected_feature),
};

const TestGroup terminal_tests = TEST_GROUP(tests);
