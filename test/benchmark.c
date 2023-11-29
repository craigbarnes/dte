#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "filetype.h"
#include "indent.h"
#include "options.h"
#include "terminal/color.h"
#include "util/arith.h"
#include "util/macros.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/time-util.h"
#include "util/xsnprintf.h"

COLD PRINTF(1)
static noreturn void fail(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    putc('\n', stderr);
    fflush(stderr);
    exit(1);
}

static void get_time(struct timespec *ts)
{
    if (clock_gettime(CLOCK_MONOTONIC, ts) != 0) {
        fail("clock_gettime() failed: %s", strerror(errno));
    }
}

static void timespec_subtract (
    const struct timespec *lhs,
    const struct timespec *rhs,
    struct timespec *res
) {
    res->tv_sec = lhs->tv_sec - rhs->tv_sec;
    res->tv_nsec = lhs->tv_nsec - rhs->tv_nsec;
    if (res->tv_nsec < 0) {
        res->tv_sec--;
        res->tv_nsec += NS_PER_SECOND;
    }
}

static uintmax_t timespec_to_ns(struct timespec *ts)
{
    if (ts->tv_sec < 0 || ts->tv_nsec < 0) {
        fail("%s(): negative timespec value", __func__);
    }

    uintmax_t sec = ts->tv_sec;
    uintmax_t ns = ts->tv_nsec;
    if (
        umax_multiply_overflows(sec, NS_PER_SECOND, &sec)
        || umax_add_overflows(ns, sec, &ns)
    ) {
        fail("%s(): %s", __func__, strerror(EOVERFLOW));
    }

    return ns;
}

PRINTF(3)
static void report(const struct timespec *start, unsigned int iters, const char *fmt, ...)
{
    struct timespec end, diff;
    get_time(&end);
    timespec_subtract(&end, start, &diff);
    uintmax_t ns = timespec_to_ns(&diff);
    char name[64];
    va_list ap;
    va_start(ap, fmt);
    xvsnprintf(name, sizeof name, fmt, ap);
    va_end(ap);
    fprintf(stderr, "   BENCH  %-24s  %9ju ns/iter\n", name, ns / iters);
}

static void do_bench_find_ft(const char *expected_ft, const char *filename)
{
    BUG_ON(expected_ft[0] == '/');
    BUG_ON(filename[0] != '/');

    PointerArray filetypes = PTR_ARRAY_INIT;
    StringView line = STRING_VIEW_INIT;
    if (!xstreq(find_ft(&filetypes, filename, line), expected_ft)) {
        fail("unexpected return value for find_ft() in %s()", __func__);
    }

    unsigned int iterations = 300000;
    struct timespec start;
    get_time(&start);

    for (unsigned int i = 0; i < iterations; i++) {
        find_ft(&filetypes, filename, line);
    }

    report(&start, iterations, "find_ft() -> %s", expected_ft);
}

static void bench_find_ft(void)
{
    do_bench_find_ft("tex", "/home/user/tex/example.tex");
    do_bench_find_ft("make", "/home/xyz/code/Makefile");
    do_bench_find_ft("c", "/usr/include/stdlib.h");
    do_bench_find_ft("config", "/etc/hosts");
}

static void do_bench_get_indent_width(const StringView *line, unsigned int tab_width)
{
    unsigned int iterations = 30000;
    unsigned int accum = 0;
    struct timespec start;
    get_time(&start);

    for (unsigned int i = 0; i < iterations; i++) {
        accum += get_indent_width(line, tab_width);
    }

    if (accum != (iterations * line->length)) {
        fail("unexpected result in %s(): %u", __func__, accum);
    }

    report(&start, iterations, "get_indent_width() <- %u", tab_width);
}

static void do_bench_get_indent_info(const LocalOptions *opts, const StringView *line)
{
    unsigned int iterations = 30000;
    unsigned int accum = 0;
    IndentInfo info = {.width = 0};
    struct timespec start;
    get_time(&start);

    for (unsigned int i = 0; i < iterations; i++) {
        info = get_indent_info(opts, line);
        accum += info.width + info.bytes;
    }

    if (accum != iterations * line->length * 2) {
        fail("unexpected result in %s(): %u", __func__, accum);
    }

    report(&start, iterations, "get_indent_info() <- %u", opts->indent_width);
}

static void bench_get_indent(void)
{
    char buf[64];
    size_t n = sizeof(buf);
    StringView line = string_view(memset(buf, ' ', n), n);

    static_assert(TAB_WIDTH_MAX == 8);
    for (unsigned int tw = 1; tw <= 8; tw++) {
        do_bench_get_indent_width(&line, tw);
    }

    static_assert(INDENT_WIDTH_MAX == 8);
    LocalOptions options = {.expand_tab = true, .tab_width = 8};
    for (unsigned int iw = 1; iw <= 8; iw++) {
        options.indent_width = iw;
        do_bench_get_indent_info(&options, &line);
    }
}

static void bench_parse_rgb(void)
{
    static const char colors[][7] = {
        "\003000",
        "\003fff",
        "\006123456",
        "\006abcdef",
        "\006ABCDEF",
        "\006012345",
        "\006111111",
        "\006ffffff",
        "\006223344",
        "\006fefefe",
        "\006123456",
        "\006aaaaaa",
        "\006000000",
        "\006a4b6c9",
        "\006fedcba",
        "\006999999",
    };

    static_assert(IS_POWER_OF_2(ARRAYLEN(colors)));
    unsigned int iterations = 30000;
    unsigned int accum = 0;
    struct timespec start;
    get_time(&start);

    for (unsigned int i = 0; i < iterations; i++) {
        const char *s = colors[i % ARRAYLEN(colors)];
        accum |= parse_rgb(s + 1, s[0]);
    }

    if (accum != COLOR_RGB(0xFFFFFF)) {
        fail("unexpected result in %s(): %u", __func__, accum);
    }

    report(&start, iterations, "parse_rgb()");
}

int main(void)
{
    bench_find_ft();
    bench_get_indent();
    bench_parse_rgb();
    return 0;
}
