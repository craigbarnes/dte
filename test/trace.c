#include "test.h"
#include "trace.h"

static void test_trace_flags_from_str(TestContext *ctx)
{
    EXPECT_UINT_EQ(trace_flags_from_str("status"), TRACE_STATUSLINE);
    EXPECT_UINT_EQ(trace_flags_from_str(",x,, ,status,,"), TRACE_STATUSLINE);
    EXPECT_UINT_EQ(trace_flags_from_str("command,input"), TRACE_COMMAND | TRACE_INPUT);
    EXPECT_UINT_EQ(trace_flags_from_str("command,inpu"), TRACE_COMMAND);
    EXPECT_UINT_EQ(trace_flags_from_str(""), 0);
    EXPECT_UINT_EQ(trace_flags_from_str(","), 0);
    EXPECT_UINT_EQ(trace_flags_from_str("a"), 0);
}

static const TestEntry tests[] = {
    TEST(test_trace_flags_from_str),
};

const TestGroup trace_tests = TEST_GROUP(tests);
