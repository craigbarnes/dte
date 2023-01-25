#include "test.h"
#include "terminal/color.h"
#include "terminal/cursor.h"
#include "terminal/key.h"
#include "terminal/linux.h"
#include "terminal/osc52.h"
#include "terminal/output.h"
#include "terminal/rxvt.h"
#include "terminal/terminal.h"
#include "terminal/xterm.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/unicode.h"
#include "util/xsnprintf.h"

#define EXPECT_KEYCODE_EQ(idx, a, b, seq, seq_len) EXPECT(keycode_eq, idx, a, b, seq, seq_len)

static void expect_keycode_eq (
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
        ctx->passed++;
        return;
    }

    char a_str[KEYCODE_STR_MAX];
    char b_str[KEYCODE_STR_MAX];
    char seq_str[64];
    keycode_to_string(a, a_str);
    keycode_to_string(b, b_str);
    make_printable_mem(seq, seq_len, seq_str, sizeof seq_str);

    test_fail(
        ctx, file, line,
        "Test #%zu: key codes not equal: 0x%02x, 0x%02x (%s, %s); input: %s",
        idx, a, b, a_str, b_str, seq_str
    );
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

static void test_parse_term_color(TestContext *ctx)
{
    static const struct {
        ssize_t expected_return;
        const char *const strs[4];
        TermColor expected_color;
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
        const TermColor expected = tests[i].expected_color;
        TermColor parsed = {COLOR_INVALID, COLOR_INVALID, 0};
        char **strs = (char**)tests[i].strs;
        ssize_t n = parse_term_color(&parsed, strs, string_array_length(strs));
        IEXPECT_EQ(n, tests[i].expected_return);
        IEXPECT_EQ(parsed.fg, expected.fg);
        IEXPECT_EQ(parsed.bg, expected.bg);
        IEXPECT_EQ(parsed.attr, expected.attr);
        IEXPECT_TRUE(same_color(&parsed, &expected));
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
        IEXPECT_EQ(color_to_nearest(c, TERM_TRUE_COLOR, false), c);
        IEXPECT_EQ(color_to_nearest(c, TERM_TRUE_COLOR, true), tests[i].expected_rgb);
        IEXPECT_EQ(color_to_nearest(c, TERM_256_COLOR, false), tests[i].expected_256);
        IEXPECT_EQ(color_to_nearest(c, TERM_16_COLOR, false), tests[i].expected_16);
        IEXPECT_EQ(color_to_nearest(c, TERM_8_COLOR, false), tests[i].expected_16 & 7);
        IEXPECT_EQ(color_to_nearest(c, TERM_0_COLOR, false), COLOR_DEFAULT);
    }

    static const uint8_t color_stops[6] = {0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff};
    size_t count = 0;

    for (size_t i = 1; i < ARRAYLEN(color_stops); i++) {
        uint8_t min = color_stops[i - 1];
        uint8_t max = color_stops[i];
        uint8_t mid = min + ((max - min) / 2);
        for (unsigned int b = min; b <= max; b++, count++) {
            int32_t orig = COLOR_RGB(b);
            int32_t nearest = color_to_nearest(orig, TERM_256_COLOR, false);
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
                EXPECT_EQ(nearest, color_to_nearest(orig, TERM_TRUE_COLOR, true));
            }
            EXPECT_EQ(nearest_stop_idx, ((uint8_t)b - (b < 75 ? 7 : 35)) / 40);
        }
    }

    EXPECT_EQ(count, 255 + ARRAYLEN(color_stops) - 1);
}

static void test_term_color_to_string(TestContext *ctx)
{
    static const struct {
        const char *expected_string;
        const TermColor color;
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

    FOR_EACH_I(i, tests) {
        const char *str = term_color_to_string(&tests[i].color);
        EXPECT_STREQ(str, tests[i].expected_string);
    }

    // Ensure longest possible color string doesn't overflow the
    // static buffer in term_color_to_string()
    const TermColor color = {
        .fg = COLOR_LIGHTMAGENTA,
        .bg = COLOR_LIGHTMAGENTA,
        .attr = ~0U
    };
    EXPECT_EQ(strlen(term_color_to_string(&color)), 94);
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
    EXPECT_STREQ(cursor_color_to_str(COLOR_DEFAULT), "default");
    EXPECT_STREQ(cursor_color_to_str(COLOR_KEEP), "keep");
    EXPECT_STREQ(cursor_color_to_str(COLOR_RGB(0x190AFE)), "#190afe");
}

static void test_xterm_parse_key(TestContext *ctx)
{
    static const struct {
        const char *escape_sequence;
        ssize_t expected_length;
        KeyCode expected_key;
    } tests[] = {
        {"\033[Z", 3, MOD_SHIFT | KEY_TAB},
        {"\033[1;2A", 6, MOD_SHIFT | KEY_UP},
        {"\033[1;2B", 6, MOD_SHIFT | KEY_DOWN},
        {"\033[1;2C", 6, MOD_SHIFT | KEY_RIGHT},
        {"\033[1;2D", 6, MOD_SHIFT | KEY_LEFT},
        {"\033[1;2E", 6, MOD_SHIFT | KEY_BEGIN},
        {"\033[1;2F", 6, MOD_SHIFT | KEY_END},
        {"\033[1;2H", 6, MOD_SHIFT | KEY_HOME},
        {"\033[1;8H", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_HOME},
        {"\033[1;8H~", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_HOME},
        {"\033[1;8H~_", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_HOME},
        {"\033", -1, 0},
        {"\033[", -1, 0},
        {"\033]", -1, 0},
        {"\033[1", -1, 0},
        {"\033[9", -1, 0},
        {"\033[1;", -1, 0},
        {"\033[1[", 4, KEY_IGNORE},
        {"\033[1;2", -1, 0},
        {"\033[1;8", -1, 0},
        {"\033[1;9", -1, 0},
        {"\033[1;_", 5, KEY_IGNORE},
        {"\033[1;8Z", 6, KEY_IGNORE},
        {"\033O", -1, 0},
        {"\033[\033", 2, KEY_IGNORE},
        {"\033[\030", 3, KEY_IGNORE},
        {"\033[A", 3, KEY_UP},
        {"\033[B", 3, KEY_DOWN},
        {"\033[C", 3, KEY_RIGHT},
        {"\033[D", 3, KEY_LEFT},
        {"\033[E", 3, KEY_BEGIN},
        {"\033[F", 3, KEY_END},
        {"\033[H", 3, KEY_HOME},
        {"\033[L", 3, KEY_INSERT},
        {"\033[P", 3, KEY_F1},
        {"\033[Q", 3, KEY_F2},
        {"\033[R", 3, KEY_F3},
        {"\033[S", 3, KEY_F4},
        {"\033[1~", 4, KEY_HOME},
        {"\033[2~", 4, KEY_INSERT},
        {"\033[3~", 4, KEY_DELETE},
        {"\033[4~", 4, KEY_END},
        {"\033[5~", 4, KEY_PAGE_UP},
        {"\033[6~", 4, KEY_PAGE_DOWN},
        {"\033O ", 3, KEY_SPACE},
        {"\033OA", 3, KEY_UP},
        {"\033OB", 3, KEY_DOWN},
        {"\033OC", 3, KEY_RIGHT},
        {"\033OD", 3, KEY_LEFT},
        {"\033OE", 3, KEY_BEGIN},
        {"\033OF", 3, KEY_END},
        {"\033OH", 3, KEY_HOME},
        {"\033OI", 3, KEY_TAB},
        {"\033OJ", 3, KEY_IGNORE},
        {"\033OM", 3, KEY_ENTER},
        {"\033OP", 3, KEY_F1},
        {"\033OQ", 3, KEY_F2},
        {"\033OR", 3, KEY_F3},
        {"\033OS", 3, KEY_F4},
        {"\033OT", 3, KEY_IGNORE},
        {"\033OX", 3, '='},
        {"\033Oi", 3, KEY_IGNORE},
        {"\033Oj", 3, '*'},
        {"\033Ok", 3, '+'},
        {"\033Ol", 3, ','},
        {"\033Om", 3, '-'},
        {"\033On", 3, '.'},
        {"\033Oo", 3, '/'},
        {"\033Op", 3, '0'},
        {"\033Oq", 3, '1'},
        {"\033Or", 3, '2'},
        {"\033Os", 3, '3'},
        {"\033Ot", 3, '4'},
        {"\033Ou", 3, '5'},
        {"\033Ov", 3, '6'},
        {"\033Ow", 3, '7'},
        {"\033Ox", 3, '8'},
        {"\033Oy", 3, '9'},
        {"\033Oz", 3, KEY_IGNORE},
        {"\033O~", 3, KEY_IGNORE},
        {"\033[10~", 5, KEY_IGNORE},
        {"\033[11~", 5, KEY_F1},
        {"\033[12~", 5, KEY_F2},
        {"\033[13~", 5, KEY_F3},
        {"\033[14~", 5, KEY_F4},
        {"\033[15~", 5, KEY_F5},
        {"\033[16~", 5, KEY_IGNORE},
        {"\033[17~", 5, KEY_F6},
        {"\033[18~", 5, KEY_F7},
        {"\033[19~", 5, KEY_F8},
        {"\033[20~", 5, KEY_F9},
        {"\033[21~", 5, KEY_F10},
        {"\033[22~", 5, KEY_IGNORE},
        {"\033[23~", 5, KEY_F11},
        {"\033[24~", 5, KEY_F12},
        {"\033[25~", 5, KEY_F13},
        {"\033[26~", 5, KEY_F14},
        {"\033[27~", 5, KEY_IGNORE},
        {"\033[28~", 5, KEY_F15},
        {"\033[29~", 5, KEY_F16},
        {"\033[30~", 5, KEY_IGNORE},
        {"\033[31~", 5, KEY_F17},
        {"\033[34~", 5, KEY_F20},
        {"\033[35~", 5, KEY_IGNORE},
        {"\033[6;3~", 6, MOD_META | KEY_PAGE_DOWN},
        {"\033[6;5~", 6, MOD_CTRL | KEY_PAGE_DOWN},
        {"\033[6;8~", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_PAGE_DOWN},
        // xterm + `modifyOtherKeys` option
        {"\033[27;5;9~", 9, MOD_CTRL | KEY_TAB},
        {"\033[27;5;13~", 10, MOD_CTRL | KEY_ENTER},
        {"\033[27;6;13~", 10, MOD_CTRL | MOD_SHIFT | KEY_ENTER},
        // {"\033[27;2;127~", 11, MOD_CTRL | '?'},
        // {"\033[27;6;127~", 11, MOD_CTRL | '?'},
        // {"\033[27;8;127~", 11, MOD_CTRL | MOD_META | '?'},
        {"\033[27;6;82~", 10, MOD_CTRL | MOD_SHIFT | 'r'},
        {"\033[27;5;114~", 11, MOD_CTRL | 'r'},
        {"\033[27;3;82~", 10, MOD_META | 'R'},
        {"\033[27;3;114~", 11, MOD_META | 'r'},
        // {"\033[27;4;62~", 10, MOD_META | '>'},
        {"\033[27;5;46~", 10, MOD_CTRL | '.'},
        {"\033[27;3;1114111~", 15, MOD_META | UNICODE_MAX_VALID_CODEPOINT},
        {"\033[27;3;1114112~", 15, KEY_IGNORE},
        {"\033[27;999999999999999999999;123~", 31, KEY_IGNORE},
        {"\033[27;123;99999999999999999~", 27, KEY_IGNORE},
        // www.leonerd.org.uk/hacks/fixterms/
        {"\033[13;3u", 7, MOD_META | KEY_ENTER},
        {"\033[9;5u", 6, MOD_CTRL | KEY_TAB},
        {"\033[65;3u", 7, MOD_META | 'A'},
        {"\033[108;5u", 8, MOD_CTRL | 'l'},
        {"\033[127765;3u", 11, MOD_META | 127765ul},
        {"\033[1114111;3u", 12, MOD_META | UNICODE_MAX_VALID_CODEPOINT},
        {"\033[1114112;3u", 12, KEY_IGNORE},
        {"\033[11141110;3u", 13, KEY_IGNORE},
        {"\033[11141111;3u", 13, KEY_IGNORE},
        {"\033[2147483647;3u", 15, KEY_IGNORE}, // INT32_MAX
        {"\033[2147483648;3u", 15, KEY_IGNORE}, // INT32_MAX + 1
        {"\033[4294967295;3u", 15, KEY_IGNORE}, // UINT32_MAX
        {"\033[4294967296;3u", 15, KEY_IGNORE}, // UINT32_MAX + 1
        {"\033[-1;3u", 7, KEY_IGNORE},
        {"\033[-2;3u", 7, KEY_IGNORE},
        {"\033[ 2;3u", 7, KEY_IGNORE},
        {"\033[<?>2;3u", 9, KEY_IGNORE},
        {"\033[ !//.$2;3u", 12, KEY_IGNORE},
        // https://sw.kovidgoyal.net/kitty/keyboard-protocol
        {"\033[27u", 5, MOD_CTRL | '['},
        {"\033[57376u", 8, KEY_F13},
        {"\033[57382u", 8, KEY_F19},
        {"\033[57383u", 8, KEY_F20},
        {"\033[57399u", 8, '0'},
        {"\033[57405u", 8, '6'},
        {"\033[57408u", 8, '9'},
        // TODO: {"\033[57414u", 8, KEY_ENTER},
        {"\033[57415u", 8, '='},
        {"\033[57427u", 8, KEY_BEGIN},
        {"\033[3615:3620:97;6u", 17, MOD_CTRL | MOD_SHIFT | 'a'},
        {"\033[97;5u", 7, MOD_CTRL | 'a'},
        {"\033[97;5:1u", 9, MOD_CTRL | 'a'},
        {"\033[97;5:2u", 9, MOD_CTRL | 'a'},
        {"\033[97;5:3u", 9, KEY_IGNORE},
        // TODO: {"\033[97;5:u", 8, MOD_CTRL | 'a'},
        // Excess params
        {"\033[1;2;3;4;5;6;7;8;9m", 20, KEY_IGNORE},
        // XTWINOPS replies
        {"\033]ltitle\033\\", 8, KEY_IGNORE},
        {"\033]Licon\033\\", 7, KEY_IGNORE},
        {"\033]ltitle\a", 9, KEY_IGNORE},
        {"\033]Licon\a", 8, KEY_IGNORE},
    };
    FOR_EACH_I(i, tests) {
        const char *seq = tests[i].escape_sequence;
        const size_t seq_length = strlen(seq);
        BUG_ON(seq_length == 0);
        KeyCode key = 0x18;
        ssize_t parsed_length = xterm_parse_key(seq, seq_length, &key);
        ssize_t expected_length = tests[i].expected_length;
        IEXPECT_EQ(parsed_length, expected_length);
        if (parsed_length <= 0) {
            // If nothing was parsed, key should be unmodified
            IEXPECT_EQ(key, 0x18);
            continue;
        }
        EXPECT_KEYCODE_EQ(i, key, tests[i].expected_key, seq, seq_length);
        // Ensure that parsing any truncated sequence returns -1:
        key = 0x18;
        for (size_t n = expected_length - 1; n != 0; n--) {
            parsed_length = xterm_parse_key(seq, n, &key);
            IEXPECT_EQ(parsed_length, -1);
            IEXPECT_EQ(key, 0x18);
        }
    }
}

static void test_xterm_parse_key_combo(TestContext *ctx)
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
        {"1", 'R', KEY_F3},
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
        {"0", 0},
        {"1", 0},
        {"2", MOD_SHIFT},
        {"3", MOD_META},
        {"4", MOD_SHIFT | MOD_META},
        {"5", MOD_CTRL},
        {"6", MOD_SHIFT | MOD_CTRL},
        {"7", MOD_META | MOD_CTRL},
        {"8", MOD_SHIFT | MOD_META | MOD_CTRL},
        {"9", MOD_META},
        {"10", MOD_META | MOD_SHIFT},
        {"11", MOD_META},
        {"12", MOD_META | MOD_SHIFT},
        {"13", MOD_META | MOD_CTRL},
        {"14", MOD_META | MOD_CTRL | MOD_SHIFT},
        {"15", MOD_META | MOD_CTRL},
        {"16", MOD_META | MOD_CTRL | MOD_SHIFT},
        {"17", 0},
        {"18", 0},
        {"400", 0},
    };

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
            ssize_t parsed_length = xterm_parse_key(seq, seq_length, &key);
            if (modifiers[j].mask == 0) {
                EXPECT_EQ(parsed_length, seq_length);
                EXPECT_EQ(key, KEY_IGNORE);
                continue;
            }
            KeyCode expected_key = modifiers[j].mask | templates[i].key;
            IEXPECT_EQ(parsed_length, seq_length);
            EXPECT_KEYCODE_EQ(i, key, expected_key, seq, seq_length);
            // Truncated
            key = 25;
            for (size_t n = seq_length - 1; n != 0; n--) {
                parsed_length = xterm_parse_key(seq, n, &key);
                EXPECT_EQ(parsed_length, -1);
                EXPECT_EQ(key, 25);
            }
            // Overlength
            key = 26;
            seq[seq_length++] = '~';
            parsed_length = xterm_parse_key(seq, seq_length, &key);
            IEXPECT_EQ(parsed_length, seq_length - 1);
            EXPECT_KEYCODE_EQ(i, key, expected_key, seq, seq_length);
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
                EXPECT_EQ(parsed_length, -1);
                EXPECT_EQ(key, 25);
            }

            // Truncated (single ESC)
            for (size_t n = seq_length - 2; n != 0; n--) {
                key = 25;
                parsed_length = rxvt_parse_key(seq + 1, n, &key);
                EXPECT_EQ(parsed_length, -1);
                EXPECT_EQ(key, 25);
            }
        }
    }

    static const struct {
        char seq[6];
        uint8_t seq_length;
        int8_t expected_length;
        KeyCode expected_key;
    } tests[] = {
        {STRN("\033Oa"), 3, MOD_CTRL | KEY_UP},
        {STRN("\033Ob"), 3, MOD_CTRL | KEY_DOWN},
        {STRN("\033Oc"), 3, MOD_CTRL | KEY_RIGHT},
        {STRN("\033Od"), 3, MOD_CTRL | KEY_LEFT},
        {STRN("\033O"), -1, 0},
        {STRN("\033[a"), 3, MOD_SHIFT | KEY_UP},
        {STRN("\033[b"), 3, MOD_SHIFT | KEY_DOWN},
        {STRN("\033[c"), 3, MOD_SHIFT | KEY_RIGHT},
        {STRN("\033[d"), 3, MOD_SHIFT | KEY_LEFT},
        {STRN("\033["), -1, 0},
        {STRN("\033[1;5A"), 6, MOD_CTRL | KEY_UP},
        {STRN("\033[1;5"), -1, 0},
        {STRN("\033\033[@"), 4, KEY_IGNORE},
        {STRN("\033\033["), -1, 0},
        {STRN("\033\033"), -1, 0},
        {STRN("\033"), -1, 0},
    };

    FOR_EACH_I(i, tests) {
        const char *seq = tests[i].seq;
        const size_t seq_length = tests[i].seq_length;
        KeyCode key = 0x18;
        ssize_t parsed_length = rxvt_parse_key(seq, seq_length, &key);
        KeyCode expected_key = (parsed_length > 0) ? tests[i].expected_key : 0x18;
        IEXPECT_EQ(parsed_length, tests[i].expected_length);
        EXPECT_KEYCODE_EQ(i, key, expected_key, seq, seq_length);
    }
}

static void test_linux_parse_key(TestContext *ctx)
{
    static const struct {
        char seq[6];
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
        {STRN("\033[["), -1, 0},
        {STRN("\033["), -1, 0},
        {STRN("\033"), -1, 0},
    };

    FOR_EACH_I(i, tests) {
        const char *seq = tests[i].seq;
        const size_t seq_length = tests[i].seq_length;
        KeyCode key = 0x18;
        ssize_t parsed_length = linux_parse_key(seq, seq_length, &key);
        KeyCode expected_key = (parsed_length > 0) ? tests[i].expected_key : 0x18;
        IEXPECT_EQ(parsed_length, tests[i].expected_length);
        EXPECT_KEYCODE_EQ(i, key, expected_key, seq, seq_length);
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
        {"C-a", MOD_CTRL | 'a'},
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
#if __STDC_VERSION__ >= 201112L
        {u8"ก", 0x0E01},
        {u8"C-ก", MOD_CTRL | 0x0E01},
        {u8"M-Ф", MOD_META | 0x0424},
#endif
    };

    char buf[KEYCODE_STR_MAX];
    FOR_EACH_I(i, tests) {
        const char *str = tests[i].str;
        size_t len = strlen(str);
        ASSERT_TRUE(len < sizeof(buf));
        size_t buflen = keycode_to_string(tests[i].key, buf);
        IEXPECT_STREQ(buf, str);
        IEXPECT_EQ(buflen, len);
        KeyCode parsed_key = 0;
        IEXPECT_TRUE(parse_key_string(&parsed_key, str));
        IEXPECT_EQ(parsed_key, tests[i].key);
    }

    // These combos aren't round-trippable by the code above and can't end
    // up in real bindings, since the letters are normalized to lower case
    // by parse_key_string(). We still test them nevertheless; for the sake
    // of completeness and catching unexpected changes.
    static const struct {
        const char *str;
        KeyCode key;
    } xtests[] = {
        {"C-A", MOD_CTRL | 'A'},
        {"C-S-A", MOD_CTRL | MOD_SHIFT | 'A'},
        {"M-S-A", MOD_META | MOD_SHIFT | 'A'},
        {"INVALID (0x08000000)", KEY_DETECTED_PASTE},
        {"INVALID (0x08000001)", KEY_BRACKETED_PASTE},
        {"INVALID (0xFFFFFFFF)", UINT32_MAX},
    };

    FOR_EACH_I(i, xtests) {
        const char *str = xtests[i].str;
        size_t len = strlen(str);
        ASSERT_TRUE(len < sizeof(buf));
        size_t buflen = keycode_to_string(xtests[i].key, buf);
        IEXPECT_STREQ(buf, str);
        IEXPECT_EQ(buflen, len);
    }
}

static void test_parse_key_string(TestContext *ctx)
{
    KeyCode key = 0;
    EXPECT_TRUE(parse_key_string(&key, "^I"));
    EXPECT_EQ(key, MOD_CTRL | 'i');
    EXPECT_TRUE(parse_key_string(&key, "^M"));
    EXPECT_EQ(key, MOD_CTRL | 'm');
    EXPECT_TRUE(parse_key_string(&key, "C-I"));
    EXPECT_EQ(key, MOD_CTRL | 'i');
    EXPECT_TRUE(parse_key_string(&key, "C-M"));
    EXPECT_EQ(key, MOD_CTRL | 'm');
    EXPECT_TRUE(parse_key_string(&key, "C-F1"));
    EXPECT_EQ(key, MOD_CTRL | KEY_F1);
    EXPECT_TRUE(parse_key_string(&key, "C-M-S-F20"));
    EXPECT_EQ(key, MOD_CTRL | MOD_META | MOD_SHIFT | KEY_F20);

    EXPECT_FALSE(parse_key_string(&key, "C-"));
    EXPECT_FALSE(parse_key_string(&key, "C-M-"));
    EXPECT_FALSE(parse_key_string(&key, "paste"));
    EXPECT_FALSE(parse_key_string(&key, "???"));
    EXPECT_FALSE(parse_key_string(&key, "F0"));
    EXPECT_FALSE(parse_key_string(&key, "F21"));
    EXPECT_FALSE(parse_key_string(&key, "F01"));
    EXPECT_FALSE(parse_key_string(&key, "\t"));
    EXPECT_FALSE(parse_key_string(&key, "\n"));
    EXPECT_FALSE(parse_key_string(&key, "\r"));
    EXPECT_FALSE(parse_key_string(&key, "\x1f"));
    EXPECT_FALSE(parse_key_string(&key, "\x7f"));
    EXPECT_FALSE(parse_key_string(&key, "C-\t"));
    EXPECT_FALSE(parse_key_string(&key, "C-\r"));
    EXPECT_FALSE(parse_key_string(&key, "C-\x7f"));

    // Special cases for normalization:
    EXPECT_TRUE(parse_key_string(&key, "C-A"));
    EXPECT_EQ(key, MOD_CTRL | 'a');
    EXPECT_TRUE(parse_key_string(&key, "C-S-A"));
    EXPECT_EQ(key, MOD_CTRL | MOD_SHIFT | 'a');
    EXPECT_TRUE(parse_key_string(&key, "M-A"));
    EXPECT_EQ(key, MOD_META | MOD_SHIFT | 'a');
    EXPECT_TRUE(parse_key_string(&key, "C-?"));
    EXPECT_EQ(key, MOD_CTRL | '?');
    EXPECT_TRUE(parse_key_string(&key, "C-H"));
    EXPECT_EQ(key, MOD_CTRL | 'h');
    EXPECT_TRUE(parse_key_string(&key, "M-C-?"));
    EXPECT_EQ(key, MOD_META | MOD_CTRL | '?');
    EXPECT_TRUE(parse_key_string(&key, "M-C-H"));
    EXPECT_EQ(key, MOD_META | MOD_CTRL | 'h');
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

static void test_term_add_str(TestContext *ctx)
{
    Terminal term = {.width = 80, .height = 24};
    TermOutputBuffer *obuf = &term.obuf;
    term_output_init(obuf);
    ASSERT_NONNULL(obuf->buf);

    // Fill start of buffer with zeroes, to allow using EXPECT_STREQ() below
    ASSERT_TRUE(256 <= TERM_OUTBUF_SIZE);
    memset(obuf->buf, 0, 256);

    term_add_str(obuf, "this should write nothing because obuf.width == 0");
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

    term_add_str(obuf, "1\xF0\x9F\xA7\xB2 \t xyz \t\r \xC2\xB6");
    EXPECT_EQ(obuf->count, 20);
    EXPECT_EQ(obuf->x, 17);
    EXPECT_STREQ(obuf->buf, "1\xF0\x9F\xA7\xB2 ^I xyz ^I^M \xC2\xB6");

    EXPECT_TRUE(term_put_char(obuf, 0x10FFFF));
    EXPECT_EQ(obuf->count, 24);
    EXPECT_EQ(obuf->x, 21);
    EXPECT_STREQ(obuf->buf + 20, "<" "??" ">");
    ASSERT_TRUE(clear_obuf(obuf));

    term_output_free(obuf);
}

static void test_term_clear_eol(TestContext *ctx)
{
    Terminal term = {.width = 80, .height = 24};
    TermOutputBuffer *obuf = &term.obuf;
    term_output_init(obuf);

    term.features |= TFLAG_BACK_COLOR_ERASE;
    term_output_reset(&term, 0, 80, 0);
    term_clear_eol(&term);
    EXPECT_EQ(obuf->count, 3);
    EXPECT_EQ(obuf->x, 80);
    EXPECT_MEMEQ(obuf->buf, "\033[K", 3);
    ASSERT_TRUE(clear_obuf(obuf));

    term.features &= ~TFLAG_BACK_COLOR_ERASE;
    term_output_reset(&term, 0, 80, 0);
    term_clear_eol(&term);
    EXPECT_EQ(obuf->count, 80);
    EXPECT_EQ(obuf->x, 80);
    EXPECT_EQ(obuf->buf[0], ' ');
    EXPECT_EQ(obuf->buf[79], ' ');
    ASSERT_TRUE(clear_obuf(obuf));

    term_output_free(obuf);
}

static void test_term_move_cursor(TestContext *ctx)
{
    TermOutputBuffer obuf;
    term_output_init(&obuf);
    term_move_cursor(&obuf, 12, 5);
    EXPECT_EQ(obuf.count, 7);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033[6;13H", 7);
    ASSERT_TRUE(clear_obuf(&obuf));

    term_move_cursor(&obuf, 0, 22);
    EXPECT_EQ(obuf.count, 5);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033[23H", 5);
    ASSERT_TRUE(clear_obuf(&obuf));

    term_output_free(&obuf);
}

static void test_term_set_bytes(TestContext *ctx)
{
    Terminal term = {
        .width = 80,
        .height = 24,
        .features = TFLAG_ECMA48_REPEAT,
    };

    TermOutputBuffer *obuf = &term.obuf;
    term_output_init(obuf);
    ASSERT_TRUE(clear_obuf(obuf));
    term_output_reset(&term, 0, 80, 0);

    term_set_bytes(&term, 'x', 40);
    EXPECT_EQ(obuf->count, 6);
    EXPECT_EQ(obuf->x, 40);
    EXPECT_MEMEQ(obuf->buf, "x\033[39b", 6);
    ASSERT_TRUE(clear_obuf(obuf));

    term_set_bytes(&term, '-', 5);
    EXPECT_EQ(obuf->count, 5);
    EXPECT_EQ(obuf->x, 5);
    EXPECT_MEMEQ(obuf->buf, "-----", 5);
    ASSERT_TRUE(clear_obuf(obuf));

    term_set_bytes(&term, '\n', 8);
    EXPECT_EQ(obuf->count, 8);
    EXPECT_EQ(obuf->x, 8);
    EXPECT_MEMEQ(obuf->buf, "\n\n\n\n\n\n\n\n", 8);
    ASSERT_TRUE(clear_obuf(obuf));

    term_output_free(obuf);
}

static void test_term_set_color(TestContext *ctx)
{
    Terminal term = {
        .width = 80,
        .height = 24,
        .color_type = TERM_TRUE_COLOR,
        .ncv_attributes = 0,
    };

    TermOutputBuffer *obuf = &term.obuf;
    term_output_init(obuf);

    TermColor c = {
        .fg = COLOR_RED,
        .bg = COLOR_YELLOW,
        .attr = ATTR_BOLD | ATTR_REVERSE,
    };

    term_set_color(&term, &c);
    EXPECT_EQ(obuf->count, 14);
    EXPECT_EQ(obuf->x, 0);
    EXPECT_MEMEQ(obuf->buf, "\033[0;1;7;31;43m", 14);
    ASSERT_TRUE(clear_obuf(obuf));

    c.attr = 0;
    c.fg = COLOR_RGB(0x12ef46);
    term_set_color(&term, &c);
    EXPECT_EQ(obuf->count, 22);
    EXPECT_EQ(obuf->x, 0);
    EXPECT_MEMEQ(obuf->buf, "\033[0;38;2;18;239;70;43m", 22);
    ASSERT_TRUE(clear_obuf(obuf));

    c.fg = 144;
    c.bg = COLOR_DEFAULT;
    term_set_color(&term, &c);
    EXPECT_EQ(obuf->count, 13);
    EXPECT_EQ(obuf->x, 0);
    EXPECT_MEMEQ(obuf->buf, "\033[0;38;5;144m", 13);
    ASSERT_TRUE(clear_obuf(obuf));

    term_output_free(obuf);
}

static void test_term_osc52_copy(TestContext *ctx)
{
    TermOutputBuffer obuf;
    term_output_init(&obuf);

    EXPECT_TRUE(term_osc52_copy(&obuf, STRN("foobar"), true, true));
    EXPECT_EQ(obuf.count, 18);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033]52;pc;Zm9vYmFy\033\\", 18);
    ASSERT_TRUE(clear_obuf(&obuf));

    EXPECT_TRUE(term_osc52_copy(&obuf, STRN("\xF0\x9F\xA5\xA3"), false, true));
    EXPECT_EQ(obuf.count, 17);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033]52;p;8J+low==\033\\", 17);
    ASSERT_TRUE(clear_obuf(&obuf));

    EXPECT_TRUE(term_osc52_copy(&obuf, STRN(""), true, false));
    EXPECT_EQ(obuf.count, 9);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033]52;c;\033\\", 9);
    ASSERT_TRUE(clear_obuf(&obuf));

    term_output_free(&obuf);
}

static void test_term_set_cursor_style(TestContext *ctx)
{
    Terminal term = {
        .width = 80,
        .height = 24,
        .color_type = TERM_TRUE_COLOR,
    };

    TermCursorStyle style = {
        .type = CURSOR_STEADY_BAR,
        .color = COLOR_RGB(0x22AACC),
    };

    static const char expected[] = "\033[6 q\033]12;rgb:22/aa/cc\033\\";
    size_t expected_len = sizeof(expected) - 1;
    ASSERT_EQ(expected_len, 24);

    TermOutputBuffer *obuf = &term.obuf;
    term_output_init(obuf);
    memset(obuf->buf, '@', expected_len + 16);

    term_set_cursor_style(&term, style);
    EXPECT_EQ(obuf->count, expected_len);
    EXPECT_EQ(obuf->x, 0);
    EXPECT_MEMEQ(obuf->buf, expected, expected_len);
    EXPECT_EQ(obuf->cursor_style.type, style.type);
    EXPECT_EQ(obuf->cursor_style.color, style.color);
    ASSERT_TRUE(clear_obuf(obuf));

    term_output_free(obuf);
}

static void test_term_restore_cursor_style(TestContext *ctx)
{
    Terminal term = {
        .width = 80,
        .height = 24,
        .color_type = TERM_TRUE_COLOR,
    };

    static const char expected[] = "\033[0 q\033]112\033\\";
    size_t expected_len = sizeof(expected) - 1;
    ASSERT_EQ(expected_len, 12);

    TermOutputBuffer *obuf = &term.obuf;
    term_output_init(obuf);
    memset(obuf->buf, '@', expected_len + 16);

    term_restore_cursor_style(&term);
    EXPECT_EQ(obuf->count, expected_len);
    EXPECT_EQ(obuf->x, 0);
    EXPECT_MEMEQ(obuf->buf, expected, expected_len);
    ASSERT_TRUE(clear_obuf(obuf));

    term_output_free(obuf);
}

static const TestEntry tests[] = {
    TEST(test_parse_rgb),
    TEST(test_parse_term_color),
    TEST(test_color_to_nearest),
    TEST(test_term_color_to_string),
    TEST(test_cursor_mode_from_str),
    TEST(test_cursor_type_from_str),
    TEST(test_cursor_color_from_str),
    TEST(test_cursor_color_to_str),
    TEST(test_xterm_parse_key),
    TEST(test_xterm_parse_key_combo),
    TEST(test_rxvt_parse_key),
    TEST(test_linux_parse_key),
    TEST(test_keycode_to_string),
    TEST(test_parse_key_string),
    TEST(test_term_add_str),
    TEST(test_term_clear_eol),
    TEST(test_term_move_cursor),
    TEST(test_term_set_bytes),
    TEST(test_term_set_color),
    TEST(test_term_osc52_copy),
    TEST(test_term_set_cursor_style),
    TEST(test_term_restore_cursor_style),
};

const TestGroup terminal_tests = TEST_GROUP(tests);
