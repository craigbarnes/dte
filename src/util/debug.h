#ifndef UTIL_DEBUG_H
#define UTIL_DEBUG_H

#include <stddef.h> // unreachable()
#include "macros.h"

#ifndef DEBUG
    #define DEBUG 0
#endif

// This is similar to the Linux kernel macro of the same name, or to the
// ISO C assert() macro with the condition inverted
#define BUG_ON(a) \
    IGNORE_WARNING("-Wtautological-compare") \
    if (unlikely(a)) {BUG("%s", #a);} \
    UNIGNORE_WARNINGS

#if GNUC_AT_LEAST(4, 5) || HAS_BUILTIN(__builtin_unreachable)
    // https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-_005f_005fbuiltin_005funreachable
    // https://clang.llvm.org/docs/LanguageExtensions.html#builtin-unreachable
    #define UNREACHABLE() __builtin_unreachable()
#elif __STDC_VERSION__ >= 202311L
    // ISO C23 §7.21.1
    #define UNREACHABLE() unreachable()
#else
    #define UNREACHABLE()
#endif

#if DEBUG >= 1
    // In DEBUG builds, BUG() cleans up the terminal, prints an error
    // message and then calls abort(3)
    #define DEBUG_ASSERTIONS_ENABLED 1
    #define BUG(...) bug(__FILE__, __LINE__, __func__, __VA_ARGS__)
    #define UNITTEST_ATTR CONSTRUCTOR
    noreturn void bug(const char *file, int line, const char *func, const char *fmt, ...) COLD PRINTF(4);
#else
    // In non-DEBUG builds, BUG() expands to UNREACHABLE(), which may
    // allow the compiler to optimize more aggressively
    #define DEBUG_ASSERTIONS_ENABLED 0
    #define BUG(...) UNREACHABLE()
    #define UNITTEST_ATTR UNUSED
#endif

// When used as UNITTEST{...}, any code inside the braces will be run as
// a constructor function at startup (in DEBUG builds, if the GCC/Clang
// `constructor` attribute is supported). This can be useful for unit
// testing `static` helper functions, as an alternative to making them
// `extern` and testing via test/main.c. The idea was somewhat inspired
// by D-Lang's unittest{} feature (https://dlang.org/spec/unittest.html).
#define UNITTEST UNITTEST_ATTR static void XPASTE(unittest_, COUNTER)(void)

// This environment variable is used by e.g. Valgrind to replace malloc(), etc.
// See the various setup_client_env() functions in the following files:
// https://sourceware.org/git/?p=valgrind.git;a=tree;f=coregrind/m_initimg;h=b1ef93371d8647b79e99076fab541dc6447cbcb2;hb=797c4b049e7b49176f4f155e00353837e4197d69
static inline const char *ld_preload_env_var(void)
{
#ifdef __APPLE__
    return "DYLD_INSERT_LIBRARIES";
#else
    return "LD_PRELOAD";
#endif
}

typedef void (*CleanupHandler)(void);

noreturn void fatal_error(const char *msg, int err) COLD NONNULL_ARGS;
void set_fatal_error_cleanup_handler(CleanupHandler handler);
void fatal_error_cleanup(void);

#endif
