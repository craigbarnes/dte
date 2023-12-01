#include "test.h"
#include "status.h"
#include "util/utf8.h"

static void test_sf_format(TestContext *ctx)
{
    Buffer buffer = {
        .encoding = {.type = UTF8, .name = "UTF-8"},
        .options = {.filetype = "none"},
    };

    Block *block = block_new(1);
    list_init(&buffer.blocks);
    list_add_before(&block->node, &buffer.blocks);

    View view = {
        .buffer = &buffer,
        .cursor = block_iter(&buffer),
    };

    GlobalOptions opts = {.case_sensitive_search = CSS_FALSE};
    Window window = {.view = &view};
    view.window = &window;

    char buf[64];
    size_t width = sf_format(&window, &opts, INPUT_NORMAL, buf, sizeof buf, "%% %n%s%y%s%Y%S%f%s%m%s%r... %E %t%S%N");
    EXPECT_EQ(width, 35);
    EXPECT_STREQ(buf, "% LF 1 0   (No name) ... UTF-8 none");

    width = sf_format(&window, &opts, INPUT_NORMAL, buf, sizeof buf, "%b%s%n%s%N%s%r%s%o");
    EXPECT_EQ(width, 7);
    EXPECT_STREQ(buf, " LF INS");

    buffer.bom = true;
    buffer.crlf_newlines = true;
    buffer.temporary = true;
    buffer.options.overwrite = true;
    width = sf_format(&window, &opts, INPUT_NORMAL, buf, sizeof buf, "%b%s%n%s%N%s%r%s%o");
    EXPECT_EQ(width, 21);
    EXPECT_STREQ(buf, "BOM CRLF CRLF TMP OVR");

    width = sf_format(&window, &opts, INPUT_SEARCH, buf, sizeof buf, "%M");
    EXPECT_EQ(width, 24);
    EXPECT_STREQ(buf, "[case-sensitive = false]");

    opts.case_sensitive_search = CSS_AUTO;
    width = sf_format(&window, &opts, INPUT_SEARCH, buf, sizeof buf, "%M");
    EXPECT_EQ(width, 23);
    EXPECT_STREQ(buf, "[case-sensitive = auto]");

    opts.case_sensitive_search = CSS_TRUE;
    width = sf_format(&window, &opts, INPUT_SEARCH, buf, sizeof buf, "%M%M%M%M%M%M%M");
    EXPECT_TRUE(width >= 46);
    ASSERT_TRUE(width < sizeof(buf));
    ASSERT_NONNULL(memchr(buf, '\0', sizeof(buf)));
    EXPECT_TRUE(str_has_prefix(buf, "[case-sensitive = true][case-sensitive = true]"));

    static const char expected[][8] = {
        ['%'] = "%",
        ['E'] = "UTF-8",
        ['N'] = "CRLF",
        ['X'] = "1",
        ['Y'] = "0",
        ['b'] = "BOM",
        ['f'] = "12\xF0\x9F\x91\xBD", // 👽 (U+1F47D)
        ['n'] = "CRLF",
        ['o'] = "OVR",
        ['p'] = "All",
        ['r'] = "TMP",
        ['t'] = "none",
        ['x'] = "1",
        ['y'] = "1",
    };

    buffer.display_filename = (char*)expected['f'];
    EXPECT_EQ(u_str_width(expected['f']), 4);

    char fmt[4] = "%_";
    for (size_t i = 0; i < ARRAYLEN(expected); i++) {
        fmt[1] = (char)i;
        size_t err = statusline_format_find_error(fmt);
        if (err) {
            EXPECT_UINT_EQ(err, 1);
            continue;
        }
        width = sf_format(&window, &opts, INPUT_NORMAL, buf, sizeof(buf), fmt);
        ASSERT_NONNULL(memchr(buf, '\0', sizeof(buf)));
        IEXPECT_STREQ(buf, expected[i]);
        IEXPECT_EQ(width, u_str_width(expected[i]));
    }

    block_free(block);
}

static const TestEntry tests[] = {
    TEST(test_sf_format),
};

const TestGroup status_tests = TEST_GROUP(tests);
