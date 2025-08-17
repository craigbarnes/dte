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

static void get_time(TestContext *ctx, struct timespec *ts)
{
    if (unlikely(ctx->timing && clock_gettime(CLOCK_MONOTONIC, ts) != 0)) {
        perror("clock_gettime");
        ctx->timing = false;
    }
}

static void print_timing(const TestContext *ctx, const struct timespec ts[2])
{
    double ms = timespec_to_fp_milliseconds(timespec_subtract(&ts[1], &ts[0]));
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

    struct timespec times[2];
    get_time(ctx, &times[0]);

    for (const TestEntry *t = g->tests, *end = t + g->nr_tests; t < end; t++) {
        if (unlikely(!t->name || !t->func)) {
            fputs("Error: TestEntry with NULL name or function\n", stderr);
            abort();
        }

        unsigned int prev_passed = ctx->passed;
        unsigned int prev_failed = ctx->failed;
        t->func(ctx);
        get_time(ctx, &times[1]);
        unsigned int passed = ctx->passed - prev_passed;
        unsigned int failed = ctx->failed - prev_failed;

        if (ctx->quiet && failed == 0) {
            continue;
        }

        fprintf(stderr, "   CHECK  %-30s  %5u passed", t->name, passed);

        if (unlikely(failed > 0)) {
            fprintf(stderr, " %s%4u FAILED%s", ctx->boldred, failed, ctx->sgr0);
        }

        if (ctx->timing) {
            print_timing(ctx, times);
            times[0] = times[1];
        }

        fputc('\n', stderr);
    }

    ctx->nfuncs += g->nr_tests;
    ctx->ngroups += 1;
}

static const char usage[] =
    "Usage: %s [-%s]\n\n"
    "Options:\n"
    "   -c  Enable colored output (on by default, if stderr is a tty and $NO_COLOR is unset)\n"
    "   -C  Disable colored output\n"
    "   -q  Disable all output, except for failing tests\n"
    "   -Q  Reverse the effects of -q\n"
    "   -t  Show timing information in output\n"
    "   -T  Reverse the effects of -t\n"
    "   -h  Display help summary and exit\n"
    "\n";

int main(int argc, char *argv[])
{
    static const char optstring[] = "cCqQtTh";
    const char *prog = progname(argc, argv, "test");
    bool color = isatty(STDERR_FILENO) && !xgetenv("NO_COLOR");

    TestContext ctx = {
        .timing = false,
        .boldred = "\033[1;31m",
        .yellow = "\033[33m",
        .cyan = "\033[36m",
        .dim = "\033[2m",
        .sgr0 = "\033[0m",
    };

    for (int ch; (ch = getopt(argc, argv, optstring)) != -1; ) {
        switch (ch) {
            case 'c': color = true; break;
            case 'C': color = false; break;
            case 't': ctx.timing = true; break;
            case 'T': ctx.timing = false; break;
            case 'q': ctx.quiet = true; break;
            case 'Q': ctx.quiet = false; break;
            case 'h': return ec_printf_ok(usage, prog, optstring);
            default: return EC_USAGE_ERROR;
        }
    }

    ctx.timing = !ctx.quiet && ctx.timing;
    setvbuf(stderr, NULL, _IOLBF, 0);

    if (!color) {
        ctx.boldred[0] = '\0';
        ctx.yellow[0] = '\0';
        ctx.cyan[0] = '\0';
        ctx.dim[0] = '\0';
        ctx.sgr0[0] = '\0';
    }

    // Several tests load data from the filesystem and depend on $PWD
    // being the project root directory, so perform a sanity test and
    // show a clear error message if not as expected
    if (unlikely(access("test/data/crlf.dterc", F_OK) != 0)) {
        return ec_usage_error (
            "test runner ('%s') started with incorrect working directory",
            prog
        );
    }

    struct timespec times[2];
    get_time(&ctx, &times[0]);

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

    get_time(&ctx, &times[1]);

    if (ctx.quiet && ctx.failed == 0) {
        return 0;
    }

    const char *red = ctx.failed ? ctx.boldred : "";
    const char *sgr0 = ctx.failed ? ctx.sgr0 : "";
    fprintf (
        stderr,
        "\n   TOTAL  %u passed, %s%u failed%s",
        ctx.passed, red, ctx.failed, sgr0
    );

    if (ctx.timing) {
        fprintf(stderr, " (%u functions, %u groups)", ctx.nfuncs, ctx.ngroups);
        print_timing(&ctx, times);
    }

    fputs("\n\n", stderr);
    return ctx.failed ? 1 : 0;
}
