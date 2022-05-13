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
        .actions = {
            [STDIN_FILENO] = SPAWN_PIPE,
            [STDOUT_FILENO] = SPAWN_PIPE,
            [STDERR_FILENO] = SPAWN_PIPE,
        },
    };

    static_assert(ARRAYLEN(sc.actions) == 3);
    static_assert(ARRAYLEN(sc.outputs) == 2);

    String *out = &sc.outputs[0];
    String *err = &sc.outputs[1];
    EXPECT_EQ(spawn(&sc), 0);
    EXPECT_EQ(out->len, 7);
    EXPECT_EQ(err->len, 4);
    EXPECT_STREQ(string_borrow_cstring(out), "IN-OUT\n");
    EXPECT_STREQ(string_borrow_cstring(err), "ERR\n");
    string_clear(out);
    string_clear(err);

    sc.actions[STDIN_FILENO] = SPAWN_NULL;
    sc.actions[STDERR_FILENO] = SPAWN_NULL;
    EXPECT_EQ(spawn(&sc), 0);
    EXPECT_EQ(out->len, 4);
    EXPECT_EQ(err->len, 0);
    EXPECT_STREQ(string_borrow_cstring(out), "OUT\n");
    string_clear(out);
    string_clear(err);

    args[2] = "printf 'xyz 123'; exit 37";
    EXPECT_EQ(spawn(&sc), 37);
    EXPECT_EQ(out->len, 7);
    EXPECT_EQ(err->len, 0);
    EXPECT_STREQ(string_borrow_cstring(out), "xyz 123");
    string_clear(out);
    string_clear(err);

    // Make sure zero-length input with one SPAWN_PIPE action
    // doesn't deadlock
    args[2] = "cat >/dev/null";
    sc.input.length = 0;
    sc.actions[STDIN_FILENO] = SPAWN_PIPE;
    sc.actions[STDOUT_FILENO] = SPAWN_NULL;
    sc.actions[STDERR_FILENO] = SPAWN_NULL;
    EXPECT_EQ(spawn(&sc), 0);
    string_free(out);
    string_free(err);
}

static const TestEntry tests[] = {
    TEST(test_spawn),
};

const TestGroup spawn_tests = TEST_GROUP(tests);
