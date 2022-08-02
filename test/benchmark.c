#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "filetype.h"
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

static void timespec_subtract(struct timespec *lhs, struct timespec *rhs, struct timespec *res)
{
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

static void print_result(const char *name, struct timespec *diff, unsigned int iters)
{
    uintmax_t ns = timespec_to_ns(diff);
    fprintf(stderr, "   BENCH  %-30s  %9ju ns/iter\n", name, ns / iters);
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
    struct timespec start, end, diff;
    get_time(&start);

    for (unsigned int i = 0; i < iterations; i++) {
        find_ft(&filetypes, filename, line);
    }

    get_time(&end);
    timespec_subtract(&end, &start, &diff);

    char name[64];
    xsnprintf(name, sizeof name, "find_ft() -> %s", expected_ft);
    print_result(name, &diff, iterations);
}

static void bench_find_ft(void)
{
    do_bench_find_ft("tex", "/home/user/tex/example.tex");
    do_bench_find_ft("make", "/home/xyz/code/Makefile");
    do_bench_find_ft("c", "/usr/include/stdlib.h");
    do_bench_find_ft("config", "/etc/hosts");
}

int main(void)
{
    bench_find_ft();
    return 0;
}
