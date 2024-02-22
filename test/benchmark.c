#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "command/serialize.h"
#include "filetype.h"
#include "indent.h"
#include "options.h"
#include "terminal/color.h"
#include "util/arith.h"
#include "util/macros.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/time-util.h"
#include "util/utf8.h"
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

#define CHECK_RESULT(val, expected) check(__FILE__, __LINE__, val, expected)

static void check(const char *file, int line, uintmax_t a, uintmax_t b)
{
    if (unlikely(a != b)) {
        fail("%s:%d: Result check failed: %ju != %ju", file, line, a, b);
    }
}

static void get_time(struct timespec *ts)
{
    if (clock_gettime(CLOCK_MONOTONIC, ts) != 0) {
        fail("clock_gettime() failed: %s", strerror(errno));
    }
}

static uintmax_t timespec_to_ns(const struct timespec *ts)
{
    if (ts->tv_sec < 0 || ts->tv_nsec < 0) {
        fail("%s(): Negative timespec value", __func__);
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
    fprintf(stderr, "   BENCH  %-29s  %9ju ns/iter\n", name, ns / iters);
}

static void do_bench_find_ft(const char *expected_ft, const char *filename)
{
    BUG_ON(expected_ft[0] == '/');
    BUG_ON(filename[0] != '/');

    PointerArray filetypes = PTR_ARRAY_INIT;
    StringView line = STRING_VIEW_INIT;
    if (!xstreq(find_ft(&filetypes, filename, line), expected_ft)) {
        fail("Unexpected return value for find_ft() in %s()", __func__);
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

    CHECK_RESULT(accum, iterations * line->length);
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

    CHECK_RESULT(accum, iterations * line->length * 2);
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

    CHECK_RESULT(accum, COLOR_RGB(0xFFFFFF));
    report(&start, iterations, "parse_rgb()");
}

static void bench_string_append_escaped_arg(void)
{
    static const char args[][32] = {
        "asdas893843\t\nxsd asasd\"",
        "/path with spaces",
        "hello world",
        " \t\r\n\x1F\x7F",
        "/path/with/no/spaces",
        "need \t\ndquotes",
        "zas9k5t4 x; e sfgsdf sfh \" '\n",
        "†•…‰™œŠŸž \033[P",
    };

    static_assert(IS_POWER_OF_2(ARRAYLEN(args)));
    unsigned int iterations = 30000;
    size_t accum = 0;
    String buf = STRING_INIT;
    struct timespec start;
    get_time(&start);

    for (unsigned int i = 0; i < iterations; i++) {
        string_append_escaped_arg(&buf, args[i % ARRAYLEN(args)], true);
        accum |= buf.len;
        string_clear(&buf);
    }

    CHECK_RESULT(accum, 63);
    report(&start, iterations, "string_append_escaped_arg()");
    string_free(&buf);
}

static void bench_u_set_char(void)
{
    unsigned int iterations = 250000;
    size_t accum = 0;
    char buf[UTF8_MAX_SEQ_LEN];
    struct timespec start;
    get_time(&start);

    for (unsigned int i = 0; i < iterations; i++) {
        accum |= u_set_char(buf, i & 0xFFFFF);
    }

    CHECK_RESULT(accum, 7);
    report(&start, iterations, "u_set_char()");
}

static void bench_u_set_char_raw(void)
{
    unsigned int iterations = 250000;
    size_t accum = 0;
    char buf[UTF8_MAX_SEQ_LEN];
    struct timespec start;
    get_time(&start);

    for (unsigned int i = 0; i < iterations; i++) {
        accum |= u_set_char_raw(buf, i & 0xFFFFF);
    }

    CHECK_RESULT(accum, 7);
    report(&start, iterations, "u_set_char_raw()");
}

int main(void)
{
    bench_find_ft();
    bench_get_indent();
    bench_parse_rgb();
    bench_string_append_escaped_arg();
    bench_u_set_char();
    bench_u_set_char_raw();
    return 0;
}
