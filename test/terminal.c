#include <string.h>
#include "test.h"
#include "terminal/color.h"
#include "terminal/ecma48.h"
#include "terminal/key.h"
#include "terminal/output.h"
#include "terminal/rxvt.h"
#include "terminal/terminal.h"
#include "terminal/xterm.h"
#include "util/debug.h"
#include "util/unicode.h"
#include "util/xsnprintf.h"

static void test_parse_term_color(void)
{
    static const struct {
        const char *const strs[4];
        const TermColor expected_color;
    } tests[] = {
        {{"bold", "red", "yellow"}, {COLOR_RED, COLOR_YELLOW, ATTR_BOLD}},
        {{"#ff0000"}, {COLOR_RGB(0xff0000), -1, 0}},
        {{"#f00a9c", "reverse"}, {COLOR_RGB(0xf00a9c), -1, ATTR_REVERSE}},
        {{"black", "#00ffff"}, {COLOR_BLACK, COLOR_RGB(0x00ffff), 0}},
        {{"#123456", "#abcdef"}, {COLOR_RGB(0x123456), COLOR_RGB(0xabcdef), 0}},
        {{"red", "strikethrough"}, {COLOR_RED, -1, ATTR_STRIKETHROUGH}},
        {{"5/5/5"}, {231, COLOR_DEFAULT, 0}},
        {{"1/3/0", "0/5/2", "italic"}, {70, 48, ATTR_ITALIC}},
        {{"-1", "-2"}, {COLOR_DEFAULT, COLOR_KEEP, 0}},
        {{"keep", "red", "keep"}, {-2, COLOR_RED, ATTR_KEEP}},
        {{"bold", "blink"}, {-1, -1, ATTR_BOLD | ATTR_BLINK}},
        {{"0", "255", "underline"}, {COLOR_BLACK, 255, ATTR_UNDERLINE}},
        {{"white", "green", "dim"}, {COLOR_WHITE, COLOR_GREEN, ATTR_DIM}},
        {{"white", "green", "lowintensity"}, {COLOR_WHITE, COLOR_GREEN, ATTR_DIM}},
        {{"lightred", "lightyellow"}, {COLOR_LIGHTRED, COLOR_LIGHTYELLOW, 0}},
        {{"darkgray", "lightgreen"}, {COLOR_DARKGRAY, COLOR_LIGHTGREEN, 0}},
        {{"lightblue", "lightcyan"}, {COLOR_LIGHTBLUE, COLOR_LIGHTCYAN, 0}},
        {{"lightmagenta"}, {COLOR_LIGHTMAGENTA, COLOR_DEFAULT, 0}},
        {{"keep", "254", "keep"}, {COLOR_KEEP, 254, ATTR_KEEP}},
        {{"red", "green", "keep"}, {COLOR_RED, COLOR_GREEN, ATTR_KEEP}},
        {{"1", "2", "invisible"}, {COLOR_RED, COLOR_GREEN, ATTR_INVIS}},
    };
    FOR_EACH_I(i, tests) {
        TermColor parsed;
        bool ok = parse_term_color(&parsed, (char**)tests[i].strs);
        IEXPECT_TRUE(ok);
        if (ok) {
            const TermColor expected = tests[i].expected_color;
            IEXPECT_EQ(parsed.fg, expected.fg);
            IEXPECT_EQ(parsed.bg, expected.bg);
            IEXPECT_EQ(parsed.attr, expected.attr);
            IEXPECT_TRUE(same_color(&parsed, &expected));
        }
    }
}

static void test_color_to_nearest(void)
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
    };
    FOR_EACH_I(i, tests) {
        const int32_t c = tests[i].input;
        IEXPECT_EQ(color_to_nearest(c, TERM_TRUE_COLOR), tests[i].expected_rgb);
        IEXPECT_EQ(color_to_nearest(c, TERM_256_COLOR), tests[i].expected_256);
        IEXPECT_EQ(color_to_nearest(c, TERM_16_COLOR), tests[i].expected_16);
        IEXPECT_EQ(color_to_nearest(c, TERM_8_COLOR), tests[i].expected_16 & 7);
        IEXPECT_EQ(color_to_nearest(c, TERM_0_COLOR), COLOR_DEFAULT);
    }
}

static void test_term_color_to_string(void)
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
    };
    FOR_EACH_I(i, tests) {
        const char *str = term_color_to_string(&tests[i].color);
        EXPECT_STREQ(str, tests[i].expected_string);
    }
}

static void test_xterm_parse_key(void)
{
    static const struct xterm_key_test {
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
        {"\033]", 0, 0},
        {"\033[1", -1, 0},
        {"\033[9", -1, 0},
        {"\033[1;", -1, 0},
        {"\033[1[", 0, 0},
        {"\033[1;2", -1, 0},
        {"\033[1;8", -1, 0},
        {"\033[1;9", -1, 0},
        {"\033[1;_", 0, 0},
        {"\033[1;8Z", 0, 0},
        {"\033O", -1, 0},
        {"\033[\033", 0, 0},
        {"\033[A", 3, KEY_UP},
        {"\033[B", 3, KEY_DOWN},
        {"\033[C", 3, KEY_RIGHT},
        {"\033[D", 3, KEY_LEFT},
        {"\033[E", 3, KEY_BEGIN},
        {"\033[F", 3, KEY_END},
        {"\033[H", 3, KEY_HOME},
        {"\033[L", 3, KEY_INSERT},
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
        {"\033OM", 3, KEY_ENTER},
        {"\033OP", 3, KEY_F1},
        {"\033OQ", 3, KEY_F2},
        {"\033OR", 3, KEY_F3},
        {"\033OS", 3, KEY_F4},
        {"\033OX", 3, '='},
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
        {"\033[10~", 0, 0},
        {"\033[11~", 5, KEY_F1},
        {"\033[12~", 5, KEY_F2},
        {"\033[13~", 5, KEY_F3},
        {"\033[14~", 5, KEY_F4},
        {"\033[15~", 5, KEY_F5},
        {"\033[16~", 0, 0},
        {"\033[17~", 5, KEY_F6},
        {"\033[18~", 5, KEY_F7},
        {"\033[19~", 5, KEY_F8},
        {"\033[20~", 5, KEY_F9},
        {"\033[21~", 5, KEY_F10},
        {"\033[22~", 0, 0},
        {"\033[23~", 5, KEY_F11},
        {"\033[24~", 5, KEY_F12},
        {"\033[25~", 0, 0},
        {"\033[6;3~", 6, MOD_META | KEY_PAGE_DOWN},
        {"\033[6;5~", 6, MOD_CTRL | KEY_PAGE_DOWN},
        {"\033[6;8~", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_PAGE_DOWN},
        // Linux console
        {"\033[[A", 4, KEY_F1},
        {"\033[[B", 4, KEY_F2},
        {"\033[[C", 4, KEY_F3},
        {"\033[[D", 4, KEY_F4},
        {"\033[[E", 4, KEY_F5},
        // rxvt
        {"\033Oa", 3, MOD_CTRL | KEY_UP},
        {"\033Ob", 3, MOD_CTRL | KEY_DOWN},
        {"\033Oc", 3, MOD_CTRL | KEY_RIGHT},
        {"\033Od", 3, MOD_CTRL | KEY_LEFT},
        {"\033[a", 3, MOD_SHIFT | KEY_UP},
        {"\033[b", 3, MOD_SHIFT | KEY_DOWN},
        {"\033[c", 3, MOD_SHIFT | KEY_RIGHT},
        {"\033[d", 3, MOD_SHIFT | KEY_LEFT},
        // xterm + `modifyOtherKeys` option
        {"\033[27;5;9~", 9, MOD_CTRL | KEY_TAB},
        {"\033[27;5;13~", 10, MOD_CTRL | KEY_ENTER},
        {"\033[27;6;13~", 10, MOD_CTRL | MOD_SHIFT |KEY_ENTER},
        {"\033[27;2;127~", 11, MOD_CTRL | '?'},
        {"\033[27;6;127~", 11, MOD_CTRL | '?'},
        {"\033[27;8;127~", 11, MOD_CTRL | MOD_META | '?'},
        {"\033[27;6;82~", 10, MOD_CTRL | 'R'},
        {"\033[27;5;114~", 11, MOD_CTRL | 'R'},
        {"\033[27;3;82~", 10, MOD_META | 'R'},
        {"\033[27;3;114~", 11, MOD_META | 'r'},
        {"\033[27;4;62~", 10, MOD_META | '>'},
        {"\033[27;5;46~", 10, MOD_CTRL | '.'},
        {"\033[27;3;1114111~", 15, MOD_META | UNICODE_MAX_VALID_CODEPOINT},
        {"\033[27;3;1114112~", 0, 0},
        {"\033[27;999999999999999999999;123~", 0, 0},
        {"\033[27;123;99999999999999999~", 0, 0},
        // www.leonerd.org.uk/hacks/fixterms/
        {"\033[13;3u", 7, MOD_META | KEY_ENTER},
        {"\033[9;5u", 6, MOD_CTRL | KEY_TAB},
        {"\033[65;3u", 7, MOD_META | 'A'},
        {"\033[108;5u", 8, MOD_CTRL | 'L'},
        {"\033[127765;3u", 11, MOD_META | 127765ul},
        {"\033[1114111;3u", 12, MOD_META | UNICODE_MAX_VALID_CODEPOINT},
        {"\033[1114112;3u", 0, 0},
        {"\033[11141110;3u", 0, 0},
        {"\033[11141111;3u", 0, 0},
        {"\033[2147483647;3u", 0, 0}, // INT32_MAX
        {"\033[2147483648;3u", 0, 0}, // INT32_MAX + 1
        {"\033[4294967295;3u", 0, 0}, // UINT32_MAX
        {"\033[4294967296;3u", 0, 0}, // UINT32_MAX + 1
        {"\033[-1;3u", 0, 0},
        {"\033[-2;3u", 0, 0},
        // kitty "full mode"
        {"\033_KpA>\033\\", 8, KEY_F6},
        {"\033_KtA>\033\\", 8, KEY_F6}, // 't' = repeat
        {"\033_KrA>\033\\", 8, KEY_IGNORE}, // 'r' = release
        {"\033_KzA>\033\\", 0, 0}, // 'z' = invalid event type
        {"\033_KpH>\033\\", 8, MOD_CTRL | MOD_META | MOD_SHIFT | KEY_F6},
        {"\033_KpI>\033\\", 8, KEY_IGNORE}, // Super bit (0x8) set in mods ('I')
        {"\033_KpA>?\\", 0, 0}, // Invalid string terminator
        {"\033_KpA>\033?", 0, 0}, // Invalid string terminator
        {"\033_KpFe\033\\", 8, MOD_CTRL | 'M'}, // Ctrl+Shift+m (Shift stripped)
        {"\033_KpAB\033\\", 8, '\''},
        {"\033_KpA-\033\\", 8, KEY_END},
        {"\033_KpA:\033\\", 8, KEY_IGNORE},
        {"\033_KpA'\033\\", 0, 0},
        {"\033_KpA~\033\\", 0, 0},
        {"\033_KpABA\033\\", 9, KEY_IGNORE},
        {"\033_KpABJ\033\\", 9, '0'},
        {"\033_KpABS\033\\", 9, '9'},
        {"\033_KpABT\033\\", 9, '.'},
        {"\033_KpABU\033\\", 9, '/'},
        {"\033_KpABV\033\\", 9, '*'},
        {"\033_KpABW\033\\", 9, '-'},
        {"\033_KpABX\033\\", 9, '+'},
        {"\033_KpABY\033\\", 9, KEY_ENTER},
        {"\033_KpABZ\033\\", 9, '='},
        {"\033_KpAB1\033\\", 0, 0},
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
        IEXPECT_EQ(key, tests[i].expected_key);
        // Ensure that parsing any truncated sequence returns -1:
        key = 0x18;
        for (size_t n = expected_length - 1; n != 0; n--) {
            parsed_length = xterm_parse_key(seq, n, &key);
            IEXPECT_EQ(parsed_length, -1);
            IEXPECT_EQ(key, 0x18);
        }
    }
}

static void test_xterm_parse_key_combo(void)
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
                EXPECT_EQ(parsed_length, 0);
                EXPECT_EQ(key, 24);
                continue;
            }
            EXPECT_EQ(parsed_length, seq_length);
            EXPECT_EQ(key, modifiers[j].mask | templates[i].key);
            // Truncated
            key = 25;
            for (size_t n = seq_length - 1; n != 0; n--) {
                parsed_length = xterm_parse_key(seq, n, &key);
                EXPECT_EQ(parsed_length, -1);
                EXPECT_EQ(key, 25);
            }
            // Overlength
            key = 26;
            seq[seq_length] = '~';
            parsed_length = xterm_parse_key(seq, seq_length + 1, &key);
            EXPECT_EQ(parsed_length, seq_length);
            EXPECT_EQ(key, modifiers[j].mask | templates[i].key);
        }
    }
}

static void test_rxvt_parse_key(void)
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

            ssize_t parsed_length = rxvt_parse_key(seq, seq_length, &key);
            EXPECT_EQ(parsed_length, seq_length);
            EXPECT_EQ(key, modifiers[j].mask | templates[i].key | MOD_META);

            parsed_length = rxvt_parse_key(seq + 1, seq_length - 1, &key);
            EXPECT_EQ(parsed_length, seq_length - 1);
            EXPECT_EQ(key, modifiers[j].mask | templates[i].key);

            parsed_length = rxvt_parse_key(seq, seq_length - 1, &key);
            EXPECT_EQ(parsed_length, -1);

            parsed_length = rxvt_parse_key(seq + 1, seq_length - 2, &key);
            EXPECT_EQ(parsed_length, -1);
        }
    }

    // Check that rxvt_parse_key() falls back to xterm_parse_key()
    KeyCode key;
    ssize_t n = rxvt_parse_key(STRN("\033[1;5A"), &key);
    EXPECT_EQ(n, 6);
    EXPECT_EQ(key, MOD_CTRL | KEY_UP);
}

static void test_keycode_to_string(void)
{
    static const struct keycode_to_string_test {
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
        {"C-A", MOD_CTRL | 'A'},
        {"M-S-{", MOD_META | MOD_SHIFT | '{'},
        {"C-S-A", MOD_CTRL | MOD_SHIFT | 'A'},
        {"F1", KEY_F1},
        {"F12", KEY_F12},
        {"M-enter", MOD_META | KEY_ENTER},
        {"M-space", MOD_META | KEY_SPACE},
        {"S-tab", MOD_SHIFT | KEY_TAB},
        {"C-M-S-F12", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_F12},
        {"C-M-S-up", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_UP},
        {"C-M-delete", MOD_CTRL | MOD_META | KEY_DELETE},
        {"C-home", MOD_CTRL | KEY_HOME},
    };
    FOR_EACH_I(i, tests) {
        const char *str = keycode_to_string(tests[i].key);
        IEXPECT_STREQ(str, tests[i].str);
        KeyCode key = 0;
        IEXPECT_TRUE(parse_key_string(&key, tests[i].str));
        IEXPECT_EQ(key, tests[i].key);
    }
    EXPECT_STREQ(keycode_to_string(KEY_PASTE), "INVALID (0x08000000)");
    EXPECT_STREQ(keycode_to_string(UINT32_MAX), "INVALID (0xFFFFFFFF)");
}

static void test_parse_key_string(void)
{
    KeyCode key = 0;
    EXPECT_TRUE(parse_key_string(&key, "^I"));
    EXPECT_EQ(key, KEY_TAB);
    EXPECT_TRUE(parse_key_string(&key, "^M"));
    EXPECT_EQ(key, KEY_ENTER);
    EXPECT_TRUE(parse_key_string(&key, "C-I"));
    EXPECT_EQ(key, KEY_TAB);
    EXPECT_TRUE(parse_key_string(&key, "C-M"));
    EXPECT_EQ(key, KEY_ENTER);

    key = 0x18;
    EXPECT_FALSE(parse_key_string(&key, "C-"));
    EXPECT_EQ(key, 0x18);
    EXPECT_FALSE(parse_key_string(&key, "C-M-"));
    EXPECT_EQ(key, 0x18);
    EXPECT_FALSE(parse_key_string(&key, "paste"));
    EXPECT_EQ(key, 0x18);
    EXPECT_FALSE(parse_key_string(&key, "???"));
    EXPECT_EQ(key, 0x18);
}

static void clear_obuf(void)
{
    ASSERT_TRUE(obuf_avail() > 8);
    memset(obuf.buf, '\0', obuf.count + 8);
    obuf.count = 0;
    obuf.x = 0;
}

static void test_term_add_str(void)
{
    EXPECT_EQ(obuf.tab, TAB_NORMAL);
    EXPECT_EQ(obuf.count, 0);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_EQ(obuf.scroll_x, 0);
    EXPECT_EQ(obuf.width, 0);

    term_add_str("this should write nothing because obuf.width == 0");
    EXPECT_EQ(obuf.count, 0);
    EXPECT_EQ(obuf.x, 0);

    term_output_reset(0, 80, 0);
    EXPECT_EQ(obuf.tab, TAB_CONTROL);
    EXPECT_EQ(obuf.tab_width, 8);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_EQ(obuf.width, 80);
    EXPECT_EQ(obuf.scroll_x, 0);
    EXPECT_EQ(terminal.width, 80);
    EXPECT_EQ(obuf.can_clear, true);

    term_add_str("1\xF0\x9F\xA7\xB2 \t xyz \t\r \xC2\xB6");
    EXPECT_EQ(obuf.count, 20);
    EXPECT_EQ(obuf.x, 17);
    EXPECT_STREQ(obuf.buf, "1\xF0\x9F\xA7\xB2 ^I xyz ^I^M \xC2\xB6");

    EXPECT_TRUE(term_put_char(0x10FFFF));
    EXPECT_EQ(obuf.count, 24);
    EXPECT_EQ(obuf.x, 21);
    EXPECT_STREQ(obuf.buf + 20, "<" "??" ">");
    clear_obuf();
}

static void test_ecma48_clear_to_eol(void)
{
    ecma48_clear_to_eol();
    EXPECT_EQ(obuf.count, 3);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033[K", 3);
    clear_obuf();
}

static void test_ecma48_move_cursor(void)
{
    ecma48_move_cursor(12, 5);
    EXPECT_EQ(obuf.count, 7);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033[6;13H", 7);
    clear_obuf();

    ecma48_move_cursor(0, 22);
    EXPECT_EQ(obuf.count, 5);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033[23H", 5);
    clear_obuf();
}

static void test_ecma48_repeat_byte(void)
{
    ecma48_repeat_byte('x', 40);
    EXPECT_EQ(obuf.count, 6);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "x\033[39b", 6);
    clear_obuf();

    ecma48_repeat_byte('-', 5);
    EXPECT_EQ(obuf.count, 5);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "-----", 5);
    clear_obuf();

    ecma48_repeat_byte('\n', 8);
    EXPECT_EQ(obuf.count, 8);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\n\n\n\n\n\n\n\n", 8);
    clear_obuf();
}

static void test_ecma48_set_color(void)
{
    const TermColorCapabilityType color_type = terminal.color_type;
    EXPECT_EQ(color_type, TERM_8_COLOR);
    EXPECT_EQ(terminal.ncv_attributes, 0);
    terminal.color_type = TERM_TRUE_COLOR;

    TermColor c = {
        .fg = COLOR_RED,
        .bg = COLOR_YELLOW,
        .attr = ATTR_BOLD | ATTR_REVERSE,
    };

    ecma48_set_color(&c);
    EXPECT_EQ(obuf.count, 14);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033[0;1;7;31;43m", 14);
    clear_obuf();

    c.attr = 0;
    c.fg = COLOR_RGB(0x12ef46);
    ecma48_set_color(&c);
    EXPECT_EQ(obuf.count, 22);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033[0;38;2;18;239;70;43m", 22);
    clear_obuf();

    c.fg = 144;
    c.bg = COLOR_DEFAULT;
    ecma48_set_color(&c);
    EXPECT_EQ(obuf.count, 13);
    EXPECT_EQ(obuf.x, 0);
    EXPECT_MEMEQ(obuf.buf, "\033[0;38;5;144m", 13);
    clear_obuf();

    terminal.color_type = color_type;
}

static const TestEntry tests[] = {
    TEST(test_parse_term_color),
    TEST(test_color_to_nearest),
    TEST(test_term_color_to_string),
    TEST(test_xterm_parse_key),
    TEST(test_xterm_parse_key_combo),
    TEST(test_rxvt_parse_key),
    TEST(test_keycode_to_string),
    TEST(test_parse_key_string),
    TEST(test_term_add_str),
    TEST(test_ecma48_clear_to_eol),
    TEST(test_ecma48_move_cursor),
    TEST(test_ecma48_repeat_byte),
    TEST(test_ecma48_set_color),
};

const TestGroup terminal_tests = TEST_GROUP(tests);
