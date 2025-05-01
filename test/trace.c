#include "test.h"
#include "trace.h"

static void test_trace_flags_from_str(TestContext *ctx)
{
    EXPECT_UINT_EQ(trace_flags_from_str("output"), TRACE_FLAG_OUTPUT);
    EXPECT_UINT_EQ(trace_flags_from_str(",x,, ,output,,"), TRACE_FLAG_OUTPUT);
    EXPECT_UINT_EQ(trace_flags_from_str("command,input"), TRACE_FLAG_COMMAND | TRACE_FLAG_INPUT);
    EXPECT_UINT_EQ(trace_flags_from_str("command,inpu"), TRACE_FLAG_COMMAND);
    EXPECT_UINT_EQ(trace_flags_from_str(""), 0);
    EXPECT_UINT_EQ(trace_flags_from_str(","), 0);
    EXPECT_UINT_EQ(trace_flags_from_str("a"), 0);
}

static const TestEntry tests[] = {
    TEST(test_trace_flags_from_str),
};

const TestGroup trace_tests = TEST_GROUP(tests);
