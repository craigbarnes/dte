#include "test.h"
#include "options.h"

static void test_common_options_offsets(void)
{
    #define GLOBAL_OFFSET(m) offsetof(GlobalOptions, m)
    #define LOCAL_OFFSET(m) offsetof(LocalOptions, m)
    #define CHECK_OFFSETS(m) EXPECT_UINT_EQ(GLOBAL_OFFSET(m), LOCAL_OFFSET(m))

    CHECK_OFFSETS(detect_indent);
    CHECK_OFFSETS(indent_width);
    CHECK_OFFSETS(tab_width);
    CHECK_OFFSETS(text_width);
    CHECK_OFFSETS(ws_error);
    CHECK_OFFSETS(auto_indent);
    CHECK_OFFSETS(editorconfig);
    CHECK_OFFSETS(emulate_tab);
    CHECK_OFFSETS(expand_tab);
    CHECK_OFFSETS(file_history);
    CHECK_OFFSETS(fsync);
    CHECK_OFFSETS(syntax);
}

DISABLE_WARNING("-Wmissing-prototypes")

void test_options(void)
{
    test_common_options_offsets();
}
