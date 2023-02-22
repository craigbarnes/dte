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
#include "util/arith.h"
#include "util/macros.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/xsnprintf.h"

enum {
    NS_PER_SECOND = 1000000000LL
};

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

static void do_bench_get_indent_width(size_t iw)
{
    const LocalOptions options = {
        .expand_tab = true,
        .indent_width = iw,
        .tab_width = 8,
    };

    char buf[64];
    size_t n = sizeof(buf);
    StringView line = string_view(memset(buf, ' ', n), n);

    unsigned int iterations = 30000;
    unsigned int accum = 0;
    struct timespec start;
    get_time(&start);

    for (unsigned int i = 0; i < iterations; i++) {
        accum += get_indent_width(&options, &line);
    }

    if (accum != (iterations * n)) {
        fail("unexpected result in %s(): %u", __func__, accum);
    }

    report(&start, iterations, "get_indent_width() <- %zu", iw);
}

static void bench_get_indent_width(void)
{
    for (unsigned int i = 1; i <= 8; i++) {
        do_bench_get_indent_width(i);
    }
}

int main(void)
{
    bench_find_ft();
    bench_get_indent_width();
    return 0;
}
