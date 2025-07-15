#include <stdlib.h>
#include "test.h"
#include "regexp.h"

static void test_regexp_escape(TestContext *ctx)
{
    static const char pat[] = "^([a-z]+|0-9{3,6}|\\.|-?.*)$";
    ASSERT_TRUE(regexp_is_valid(NULL, pat, REG_NEWLINE));
    char *escaped = regexp_escape(pat, sizeof(pat) - 1);
    EXPECT_STREQ(escaped, "\\^\\(\\[a-z]\\+\\|0-9\\{3,6}\\|\\\\\\.\\|-\\?\\.\\*\\)\\$");

    // Ensure the escaped pattern matches the original pattern string
    regex_t re;
    ASSERT_TRUE(regexp_compile(NULL, &re, escaped, REG_NEWLINE | REG_NOSUB));
    free(escaped);
    EXPECT_TRUE(regexp_exec(&re, pat, sizeof(pat) - 1, 0, NULL, 0));
    regfree(&re);
}

static const TestEntry tests[] = {
    TEST(test_regexp_escape),
};

const TestGroup regexp_tests = TEST_GROUP(tests);
