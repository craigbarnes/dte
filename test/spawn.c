#include <unistd.h>
#include "test.h"
#include "spawn.h"

static void test_spawn(TestContext *ctx)
{
    static const char *args[] = {
        "sh", "-c", "cat; echo OUT; echo ERR >&2",
        NULL
    };

    SpawnContext sc = {
        .argv = args,
        .input = STRING_VIEW("IN-"),
        .outputs = {STRING_INIT, STRING_INIT},
        .flags = SPAWN_QUIET,
    };

    SpawnAction actions[3] = {
        [STDIN_FILENO] = SPAWN_PIPE,
        [STDOUT_FILENO] = SPAWN_PIPE,
        [STDERR_FILENO] = SPAWN_PIPE,
    };

    String *out = &sc.outputs[0];
    String *err = &sc.outputs[1];
    EXPECT_EQ(spawn(&sc, actions), 0);
    EXPECT_EQ(out->len, 7);
    EXPECT_EQ(err->len, 4);
    EXPECT_STREQ(string_borrow_cstring(out), "IN-OUT\n");
    EXPECT_STREQ(string_borrow_cstring(err), "ERR\n");
    string_clear(out);
    string_clear(err);

    actions[STDIN_FILENO] = SPAWN_NULL;
    actions[STDERR_FILENO] = SPAWN_NULL;
    EXPECT_EQ(spawn(&sc, actions), 0);
    EXPECT_EQ(out->len, 4);
    EXPECT_EQ(err->len, 0);
    EXPECT_STREQ(string_borrow_cstring(out), "OUT\n");
    string_free(out);
    string_free(err);
}

static const TestEntry tests[] = {
    TEST(test_spawn),
};

const TestGroup spawn_tests = TEST_GROUP(tests);
