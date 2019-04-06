#ifndef UTIL_MACROS_H
#define UTIL_MACROS_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#error C99 compiler required
#endif

#define STRLEN(x) (sizeof("" x "") - 1)
#define PASTE(a, b) a##b
#define XPASTE(a, b) PASTE(a, b)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define DO_PRAGMA(x) _Pragma(#x)

// Calculate the number of elements in an array.
// The extra division on the third line is a trick to help prevent
// passing a pointer to the first element of an array instead of a
// reference to the array itself.
#define ARRAY_COUNT(x) ( \
    (sizeof(x) / sizeof((x)[0])) \
    / ((size_t)(!(sizeof(x) % sizeof((x)[0])))) \
)

#ifdef __GNUC__
    #define GNUC_AT_LEAST(major, minor) ( \
        (__GNUC__ > major) \
        || ((__GNUC__ == major) && (__GNUC_MINOR__ >= minor)) )
#else
    #define GNUC_AT_LEAST(major, minor) 0
#endif

// __has_extension is a Clang macro used to determine if a feature is
// available even if not standardized in the current "-std" mode.
#ifdef __has_extension
    #define HAS_EXTENSION(x) __has_extension(x)
#else
    #define HAS_EXTENSION(x) 0
#endif

#ifdef __has_attribute
    #define HAS_ATTRIBUTE(x) __has_attribute(x)
#else
    #define HAS_ATTRIBUTE(x) 0
#endif

#ifdef __has_builtin
    #define HAS_BUILTIN(x) __has_builtin(x)
#else
    #define HAS_BUILTIN(x) 0
#endif

#ifdef __has_warning
    #define HAS_WARNING(x) __has_warning(x)
#else
    #define HAS_WARNING(x) 0
#endif

#ifdef __has_feature
    #define HAS_FEATURE(x) __has_feature(x)
#else
    #define HAS_FEATURE(x) 0
#endif

#if defined(__SANITIZE_ADDRESS__) || HAS_FEATURE(address_sanitizer)
    #define ASAN_ENABLED 1
#endif

#if defined(__clang__) && HAS_FEATURE(address_sanitizer)
    #define CLANG_ASAN_ENABLED 1
#endif

#if GNUC_AT_LEAST(3, 0) || defined(__TINYC__)
    #define UNUSED __attribute__((__unused__))
#else
    #define UNUSED
#endif

#if GNUC_AT_LEAST(3, 0)
    #define ALIGNED(x) __attribute__((__aligned__(x)))
    #define MALLOC __attribute__((__malloc__))
    #define PRINTF(x) __attribute__((__format__(__printf__, (x), (x + 1))))
    #define VPRINTF(x) __attribute__((__format__(__printf__, (x), 0)))
    #define PURE __attribute__((__pure__))
    #define CONST_FN __attribute__((__const__))
    #define CONSTRUCTOR __attribute__((__constructor__))
    #define DESTRUCTOR __attribute__((__destructor__))
#else
    #define ALIGNED(x)
    #define MALLOC
    #define PRINTF(x)
    #define VPRINTF(x)
    #define PURE
    #define CONST_FN
    #define CONSTRUCTOR UNUSED
    #define DESTRUCTOR UNUSED
#endif

#define UNUSED_ARG(x) unused__ ## x UNUSED

#ifdef __COUNTER__ // Supported by GCC 4.3+ and Clang
    #define COUNTER_ __COUNTER__
#else
    #define COUNTER_ __LINE__
#endif

#if defined(DEBUG) && (DEBUG > 0)
    #define UNITTEST static void CONSTRUCTOR XPASTE(unittest_, COUNTER_)(void)
#else
    #define UNITTEST static void UNUSED XPASTE(unittest_, COUNTER_)(void)
#endif

#if GNUC_AT_LEAST(3, 0) && defined(__OPTIMIZE__)
    #define likely(x) __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x) (x)
    #define unlikely(x) (x)
#endif

#if GNUC_AT_LEAST(3, 0) && defined(__ELF__)
    #define SECTION(x) __attribute__((__section__(x)))
#else
    #define SECTION(x)
#endif

#if GNUC_AT_LEAST(3, 1) || HAS_BUILTIN(__builtin_prefetch)
    #define PREFETCH(addr, ...) __builtin_prefetch(addr, __VA_ARGS__)
#else
    #define PREFETCH(addr, ...)
#endif

#if GNUC_AT_LEAST(3, 3) || HAS_ATTRIBUTE(nonnull)
    #define NONNULL_ARGS __attribute__((__nonnull__))
    #define NONNULL_ARG(...) __attribute__((__nonnull__(__VA_ARGS__)))
#else
    #define NONNULL_ARGS
    #define NONNULL_ARG(...)
#endif

#if GNUC_AT_LEAST(3, 4) || HAS_ATTRIBUTE(warn_unused_result)
    #define WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
#else
    #define WARN_UNUSED_RESULT
#endif

#if GNUC_AT_LEAST(4, 3) || HAS_ATTRIBUTE(alloc_size)
    #define ALLOC_SIZE(...) __attribute__((__alloc_size__(__VA_ARGS__)))
#else
    #define ALLOC_SIZE(...)
#endif

#if GNUC_AT_LEAST(4, 3) || HAS_ATTRIBUTE(hot)
    #define HOT __attribute__((__hot__))
#else
    #define HOT
#endif

#if GNUC_AT_LEAST(4, 3) || HAS_ATTRIBUTE(cold)
    #define COLD __attribute__((__cold__))
#else
    #define COLD
#endif

#if GNUC_AT_LEAST(4, 5) || HAS_BUILTIN(__builtin_unreachable)
    #define UNREACHABLE() __builtin_unreachable()
#else
    #define UNREACHABLE()
#endif

#if GNUC_AT_LEAST(5, 0) || HAS_ATTRIBUTE(returns_nonnull)
    #define RETURNS_NONNULL __attribute__((__returns_nonnull__))
#else
    #define RETURNS_NONNULL
#endif

#if GNUC_AT_LEAST(6, 0) || HAS_ATTRIBUTE(target_clones)
    #define TARGET_CLONES(x) __attribute__((__target_clones__(x)))
#else
    #define TARGET_CLONES(x)
#endif

#if GNUC_AT_LEAST(8, 0) || HAS_ATTRIBUTE(nonstring)
    #define NONSTRING __attribute__((__nonstring__))
#else
    #define NONSTRING
#endif

#if defined(__x86_64__) && !defined(__SSE4_2__)
    #define FMV_SSE42 TARGET_CLONES("sse4.2,default")
#else
    #define FMV_SSE42
#endif

#define XMALLOC MALLOC RETURNS_NONNULL

#if __STDC_VERSION__ >= 201112L
    #define alignof(t) _Alignof(t)
    #define NORETURN _Noreturn
#elif GNUC_AT_LEAST(3, 0)
    #define alignof(t) __alignof__(t)
    #define NORETURN __attribute__((__noreturn__))
#else
    #define alignof(t) MIN(sizeof(t), offsetof(struct{char c; t x;}, x))
    #define NORETURN
#endif

#if (__STDC_VERSION__ >= 201112L) || HAS_EXTENSION(c_static_assert)
    #define static_assert(x) _Static_assert((x), #x)
#else
    #define static_assert(x)
#endif

#ifdef __clang__
    #define IGNORE_WARNING(wflag) \
        DO_PRAGMA(clang diagnostic push) \
        DO_PRAGMA(clang diagnostic ignored wflag)
    #define UNIGNORE_WARNINGS DO_PRAGMA(clang diagnostic pop)
#elif GNUC_AT_LEAST(4, 6)
    #define IGNORE_WARNING(wflag) \
        DO_PRAGMA(GCC diagnostic push) \
        DO_PRAGMA(GCC diagnostic ignored wflag)
    #define UNIGNORE_WARNINGS DO_PRAGMA(GCC diagnostic pop)
#else
    #define IGNORE_WARNING(wflag)
    #define UNIGNORE_WARNINGS
#endif

#endif
