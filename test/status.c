#include "test.h"
#include "status.h"

static void test_sf_format(TestContext *ctx)
{
    Buffer buffer = {
        .encoding = {.type = UTF8, .name = "UTF-8"},
        .options = {.filetype = "none"},
    };

    list_init(&buffer.blocks);
    Block *block = block_new(1);
    list_add_before(&block->node, &buffer.blocks);

    View view = {
        .buffer = &buffer,
        .cursor = {.head = &buffer.blocks, .blk = block},
    };

    GlobalOptions opts = {.case_sensitive_search = CSS_FALSE};
    Window window = {.view = &view};
    view.window = &window;

    char buf[64];
    sf_format(&window, &opts, INPUT_NORMAL, buf, sizeof buf, "%% %n%s%y%s%Y%S%f%s%m%s%r... %E %t%S%N");
    EXPECT_STREQ(buf, "% LF 1 0   (No name) ... UTF-8 none");

    sf_format(&window, &opts, INPUT_NORMAL, buf, sizeof buf, "%b%s%n%s%N%s%r%s%o");
    EXPECT_STREQ(buf, " LF INS");

    buffer.bom = true;
    buffer.crlf_newlines = true;
    buffer.temporary = true;
    buffer.options.overwrite = true;
    sf_format(&window, &opts, INPUT_NORMAL, buf, sizeof buf, "%b%s%n%s%N%s%r%s%o");
    EXPECT_STREQ(buf, "BOM CRLF CRLF TMP OVR");

    sf_format(&window, &opts, INPUT_SEARCH, buf, sizeof buf, "%M");
    EXPECT_STREQ(buf, "[case-sensitive = false]");

    opts.case_sensitive_search = CSS_AUTO;
    sf_format(&window, &opts, INPUT_SEARCH, buf, sizeof buf, "%M");
    EXPECT_STREQ(buf, "[case-sensitive = auto]");

    opts.case_sensitive_search = CSS_TRUE;
    sf_format(&window, &opts, INPUT_SEARCH, buf, sizeof buf, "%M%M%M%M%M%M%M");
    EXPECT_NONNULL(memchr(buf, '\0', sizeof(buf)));
    EXPECT_TRUE(str_has_prefix(buf, "[case-sensitive = true][case-sensitive = true]"));

    char fmt[4] = "%_";
    for (unsigned char c = '\0'; c < 'z'; c++) {
        fmt[1] = c;
        size_t err = statusline_format_find_error(fmt);
        if (err) {
            EXPECT_UINT_EQ(err, 1);
            continue;
        }
        sf_format(&window, &opts, INPUT_NORMAL, buf, sizeof(buf), fmt);
        EXPECT_NONNULL(memchr(buf, '\0', sizeof(buf)));
    }

    block_free(block);
}

static const TestEntry tests[] = {
    TEST(test_sf_format),
};

const TestGroup status_tests = TEST_GROUP(tests);
