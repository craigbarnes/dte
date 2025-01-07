#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test.h"
#include "util/exitcode.h"
#include "util/path.h"
#include "util/progname.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/time-util.h"

void init_headless_mode(TestContext *ctx);
extern const TestGroup bind_tests;
extern const TestGroup bookmark_tests;
extern const TestGroup buffer_tests;
extern const TestGroup cmdline_tests;
extern const TestGroup cmdline_tests_late;
extern const TestGroup command_tests;
extern const TestGroup config_tests;
extern const TestGroup ctags_tests;
extern const TestGroup deinit_tests;
extern const TestGroup dump_tests;
extern const TestGroup editorconfig_tests;
extern const TestGroup encoding_tests;
extern const TestGroup error_tests;
extern const TestGroup filetype_tests;
extern const TestGroup frame_tests;
extern const TestGroup history_tests;
extern const TestGroup indent_tests;
extern const TestGroup init_tests;
extern const TestGroup option_tests;
extern const TestGroup regexp_tests;
extern const TestGroup shift_tests;
extern const TestGroup spawn_tests;
extern const TestGroup status_tests;
extern const TestGroup syntax_tests;
extern const TestGroup terminal_tests;
extern const TestGroup util_tests;

static void print_timing(const TestContext *ctx, const struct timespec ts[2])
{
    struct timespec duration = timespec_subtract(&ts[1], &ts[0]);
    double ms = (double)duration.tv_sec * (double)MS_PER_SECOND;
    ms += (double)duration.tv_nsec / (double)NS_PER_MS;

    int precision = 3 - (ms >= 1000.0) - (ms >= 100.0) - (ms >= 10.0);
    fprintf(stderr, " %s(%0.*f ms)%s", ctx->dim, precision, ms, ctx->sgr0);
}

static void run_tests(TestContext *ctx, const TestGroup *g)
{
    // Note: we avoid using test.h assertions in this function, in order to
    // avoid artificially inflating ctx->passed
    if (unlikely(g->nr_tests == 0 || !g->tests)) {
        fputs("Error: TestGroup with no entries\n", stderr);
        abort();
    }

    bool timing = ctx->timing;

    for (const TestEntry *t = g->tests, *end = t + g->nr_tests; t < end; t++) {
        if (unlikely(!t->name || !t->func)) {
            fputs("Error: TestEntry with NULL name or function\n", stderr);
            abort();
        }

        unsigned int prev_passed = ctx->passed;
        unsigned int prev_failed = ctx->failed;
        struct timespec times[2];
        timing = timing && !clock_gettime(CLOCK_MONOTONIC, &times[0]);
        t->func(ctx);
        timing = timing && !clock_gettime(CLOCK_MONOTONIC, &times[1]);
        unsigned int passed = ctx->passed - prev_passed;
        unsigned int failed = ctx->failed - prev_failed;

        fprintf(stderr, "   CHECK  %-30s  %5u passed", t->name, passed);

        if (unlikely(failed > 0)) {
            fprintf(stderr, " %s%4u FAILED%s", ctx->boldred, failed, ctx->sgr0);
        }

        if (timing) {
            print_timing(ctx, times);
        }

        fputc('\n', stderr);
    }

    ctx->nfuncs += g->nr_tests;
    ctx->ngroups += 1;
}

// Change to the expected working directory, in case the test binary
// wasn't executed via `make check` (or `build/test/test`).
static void init_working_directory(int argc, char *argv[])
{
    static const char testfile[] = "test/data/crlf.dterc";
    if (likely(access(testfile, F_OK) == 0)) {
        // Working directory already correct
        return;
    }

    if (argc < 1 || !argv || !argv[0]) {
        goto error;
    }

    char dir[8192];
    StringView progdir = path_slice_dirname(argv[0]);
    if (progdir.length >= sizeof(dir) - 8) {
        goto error;
    }

    memcpy(dir, progdir.data, progdir.length);
    copyliteral(dir + progdir.length, "/../..\0");

    if (chdir(dir) != 0) {
        perror("chdir");
        goto error;
    }

    if (access(testfile, F_OK) != 0) {
        goto error;
    }

    return;

error:
    fputs("test binary executed from incorrect working directory; exiting", stderr);
    exit(1);
}

static ExitCode usage(const char *prog, bool err)
{
    fprintf(err ? stderr : stdout, "Usage: %s [-cCtTh]\n", prog);
    return err ? EC_USAGE_ERROR : EC_OK;
}

int main(int argc, char *argv[])
{
    TestContext ctx = {
        .timing = false,
        .boldred = "\033[1;31m",
        .dim = "\033[2m",
        .sgr0 = "\033[0m",
    };

    bool color = isatty(STDERR_FILENO) && !xgetenv("NO_COLOR");

    for (int ch; (ch = getopt(argc, argv, "cCtTh")) != -1; ) {
        switch (ch) {
        case 'c':
        case 'C':
            color = (ch == 'c');
            break;
        case 't':
        case 'T':
            ctx.timing = (ch == 't');
            break;
        case 'h':
        default:
            return usage(progname(argc, argv, NULL), ch != 'h');
        }
    }

    setvbuf(stderr, NULL, _IOLBF, 0);
    init_working_directory(argc, argv);

    if (!color) {
        ctx.boldred[0] = '\0';
        ctx.dim[0] = '\0';
        ctx.sgr0[0] = '\0';
    }

    struct timespec times[2];
    bool timing = ctx.timing && !clock_gettime(CLOCK_MONOTONIC, &times[0]);

    run_tests(&ctx, &init_tests);
    run_tests(&ctx, &util_tests);
    run_tests(&ctx, &indent_tests);
    run_tests(&ctx, &regexp_tests);
    run_tests(&ctx, &command_tests);
    run_tests(&ctx, &option_tests);
    run_tests(&ctx, &editorconfig_tests);
    run_tests(&ctx, &encoding_tests);
    run_tests(&ctx, &filetype_tests);
    run_tests(&ctx, &terminal_tests);
    run_tests(&ctx, &cmdline_tests);
    run_tests(&ctx, &history_tests);
    run_tests(&ctx, &frame_tests);
    run_tests(&ctx, &ctags_tests);
    run_tests(&ctx, &spawn_tests);
    run_tests(&ctx, &status_tests);

    init_headless_mode(&ctx);
    run_tests(&ctx, &config_tests);
    run_tests(&ctx, &bind_tests);
    run_tests(&ctx, &cmdline_tests_late);
    run_tests(&ctx, &buffer_tests);
    run_tests(&ctx, &shift_tests);
    run_tests(&ctx, &bookmark_tests);
    run_tests(&ctx, &syntax_tests);
    run_tests(&ctx, &dump_tests);
    run_tests(&ctx, &error_tests);
    run_tests(&ctx, &deinit_tests);

    timing = timing && !clock_gettime(CLOCK_MONOTONIC, &times[1]);

    const char *red = ctx.failed ? ctx.boldred : "";
    const char *sgr0 = ctx.failed ? ctx.sgr0 : "";
    fprintf (
        stderr,
        "\n   TOTAL  %u passed, %s%u failed%s",
        ctx.passed, red, ctx.failed, sgr0
    );

    if (timing) {
        fprintf(stderr, " (%u functions, %u groups)", ctx.nfuncs, ctx.ngroups);
        print_timing(&ctx, times);
    }

    fputs("\n\n", stderr);
    return ctx.failed ? 1 : 0;
}
