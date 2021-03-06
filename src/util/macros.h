#ifndef UTIL_MACROS_H
#define UTIL_MACROS_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#error C99 compiler required
#endif

#define STRLEN(x) (sizeof("" x "") - 1)
#define STRN(x) x,STRLEN(x)
#define PASTE(a, b) a##b
#define XPASTE(a, b) PASTE(a, b)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) MAX(a, MAX(b, c))
#define MAX4(a, b, c, d) MAX(a, MAX3(b, c, d))
#define IS_POWER_OF_2(x) (((x) > 0) && (((x) & ((x) - 1)) == 0))
#define DECIMAL_STR_MAX(T) ((sizeof(T) * 3) + 2)
#define DO_PRAGMA(x) _Pragma(#x)

// Calculate the number of elements in an array.
// The extra division on the third line is a trick to help prevent
// passing a pointer to the first element of an array instead of a
// reference to the array itself.
#define ARRAY_COUNT(x) ( \
    (sizeof(x) / sizeof((x)[0])) \
    / ((size_t)(!(sizeof(x) % sizeof((x)[0])))) \
)

#define VERCMP(x, y, cx, cy) ((cx > x) || ((cx == x) && (cy >= y)))

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
    #define GNUC_AT_LEAST(x, y) VERCMP(x, y, __GNUC__, __GNUC_MINOR__)
#else
    #define GNUC_AT_LEAST(x, y) 0
#endif

#if defined(__clang_major__) && defined(__clang_minor__)
    #define CLANG_AT_LEAST(x, y) VERCMP(x, y, __clang_major__, __clang_minor__)
#else
    #define CLANG_AT_LEAST(x, y) 0
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

// __has_extension() is a Clang macro used to determine if a feature is
// available even if not standardized in the current "-std" mode.
#ifdef __has_extension
    #define HAS_EXTENSION(x) __has_extension(x)
#else
    // Clang versions prior to 3.0 only supported __has_feature()
    #define HAS_EXTENSION(x) HAS_FEATURE(x)
#endif

#if defined(__SANITIZE_ADDRESS__) || HAS_FEATURE(address_sanitizer)
    #define ASAN_ENABLED 1
#endif

#if defined(__SANITIZE_MEMORY__) || HAS_FEATURE(memory_sanitizer)
    #define MSAN_ENABLED 1
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(unused) || defined(__TINYC__)
    #define UNUSED __attribute__((__unused__))
#else
    #define UNUSED
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(aligned)
    #define ALIGNED(x) __attribute__((__aligned__(x)))
#else
    #define ALIGNED(x)
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(malloc)
    #define MALLOC __attribute__((__malloc__))
#else
    #define MALLOC
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(pure)
    #define PURE __attribute__((__pure__))
#else
    #define PURE
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(const)
    #define CONST_FN __attribute__((__const__))
#else
    #define CONST_FN
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(constructor)
    #define CONSTRUCTOR __attribute__((__constructor__))
#else
    #define CONSTRUCTOR UNUSED
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(format)
    #define PRINTF(x) __attribute__((__format__(__printf__, (x), (x + 1))))
    #define VPRINTF(x) __attribute__((__format__(__printf__, (x), 0)))
#else
    #define PRINTF(x)
    #define VPRINTF(x)
#endif

#if (GNUC_AT_LEAST(3, 0) || HAS_BUILTIN(__builtin_expect)) && defined(__OPTIMIZE__)
    #define likely(x) __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x) (x)
    #define unlikely(x) (x)
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_BUILTIN(__builtin_constant_p) || defined(__TINYC__)
    #define IS_CT_CONSTANT(x) __builtin_constant_p(x)
#else
    #define IS_CT_CONSTANT(x) 0
#endif

#if (GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(section)) && defined(__ELF__)
    #define SECTION(x) __attribute__((__section__(x)))
#else
    #define SECTION(x)
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_EXTENSION(typeof) || defined(__TINYC__)
    #define HAS_TYPEOF 1
#endif

#if GNUC_AT_LEAST(3, 1) || HAS_BUILTIN(__builtin_prefetch)
    #define PREFETCH(addr, ...) __builtin_prefetch(addr, __VA_ARGS__)
#else
    #define PREFETCH(addr, ...)
#endif

#if GNUC_AT_LEAST(3, 1) || HAS_ATTRIBUTE(noinline)
    #define NOINLINE __attribute__((__noinline__))
#else
    #define NOINLINE
#endif

#if GNUC_AT_LEAST(3, 1) || HAS_ATTRIBUTE(always_inline)
    #define ALWAYS_INLINE __attribute__((__always_inline__))
#else
    #define ALWAYS_INLINE
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

#if GNUC_AT_LEAST(4, 0) || HAS_ATTRIBUTE(sentinel)
    #define SENTINEL __attribute__((__sentinel__))
#else
    #define SENTINEL
#endif

#if GNUC_AT_LEAST(4, 1) || HAS_ATTRIBUTE(flatten)
    #define FLATTEN __attribute__((__flatten__))
#else
    #define FLATTEN
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

#if GNUC_AT_LEAST(8, 0) || HAS_ATTRIBUTE(nonstring)
    #define NONSTRING __attribute__((__nonstring__))
#else
    #define NONSTRING
#endif

#if HAS_ATTRIBUTE(diagnose_if)
    #define DIAGNOSE_IF(x) __attribute__((diagnose_if((x), (#x), "error")))
#else
    #define DIAGNOSE_IF(x)
#endif

#define UNUSED_ARG(x) unused__ ## x UNUSED
#define XMALLOC MALLOC RETURNS_NONNULL WARN_UNUSED_RESULT
#define XSTRDUP XMALLOC NONNULL_ARGS
#define NONNULL_ARGS_AND_RETURN RETURNS_NONNULL NONNULL_ARGS

#if __STDC_VERSION__ >= 201112L
    #define alignof(t) _Alignof(t)
    #define noreturn _Noreturn
#elif GNUC_AT_LEAST(3, 0)
    #define alignof(t) __alignof__(t)
    #define noreturn __attribute__((__noreturn__))
#else
    #define alignof(t) MIN(sizeof(t), offsetof(struct{char c; t x;}, x))
    #define noreturn
#endif

#if __STDC_VERSION__ >= 201112L
    #define static_assert(x) _Static_assert((x), #x)
    #define HAS_STATIC_ASSERT 1
#elif GNUC_AT_LEAST(4, 6) || HAS_EXTENSION(c_static_assert)
    #define static_assert(x) __extension__ _Static_assert((x), #x)
    #define HAS_STATIC_ASSERT 1
#else
    #define static_assert(x)
#endif

#if GNUC_AT_LEAST(3, 1) || HAS_BUILTIN(__builtin_types_compatible_p)
    #define HAS_BUILTIN_TYPES_COMPATIBLE_P 1
#endif

#if defined(HAS_STATIC_ASSERT) && defined(HAS_BUILTIN_TYPES_COMPATIBLE_P)
    #define static_assert_compatible_types(a, b) static_assert ( \
        __builtin_types_compatible_p(__typeof__(a), __typeof__(b)) \
    )
    #define static_assert_incompatible_types(a, b) static_assert ( \
        !__builtin_types_compatible_p(__typeof__(a), __typeof__(b)) \
    )
#else
    #define static_assert_compatible_types(a, b)
    #define static_assert_incompatible_types(a, b)
#endif

#if defined(HAS_STATIC_ASSERT) && defined(HAS_TYPEOF)
    #define static_assert_offsetof(obj, field, offset) \
        static_assert(offsetof(__typeof__(obj), field) == offset)
#else
    #define static_assert_offsetof(obj, field, offset)
#endif

#if GNUC_AT_LEAST(4, 2) || defined(__clang__)
    #define DISABLE_WARNING(wflag) DO_PRAGMA(GCC diagnostic ignored wflag)
#else
    #define DISABLE_WARNING(wflag)
#endif

#if CLANG_AT_LEAST(3, 6)
    #define UNROLL_LOOP(n) DO_PRAGMA(clang loop unroll_count(n))
#elif GNUC_AT_LEAST(8, 0)
    #define UNROLL_LOOP(n) DO_PRAGMA(GCC unroll (n))
#else
    #define UNROLL_LOOP(n)
#endif

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

#ifdef __clang__
    #define IGNORE_WARNING(wflag) \
        DO_PRAGMA(clang diagnostic push) \
        DO_PRAGMA(clang diagnostic ignored "-Wunknown-pragmas") \
        DO_PRAGMA(clang diagnostic ignored "-Wunknown-warning-option") \
        DO_PRAGMA(clang diagnostic ignored wflag)
    #define UNIGNORE_WARNINGS DO_PRAGMA(clang diagnostic pop)
#elif GNUC_AT_LEAST(4, 6)
    #define IGNORE_WARNING(wflag) \
        DO_PRAGMA(GCC diagnostic push) \
        DO_PRAGMA(GCC diagnostic ignored "-Wpragmas") \
        DO_PRAGMA(GCC diagnostic ignored wflag)
    #define UNIGNORE_WARNINGS DO_PRAGMA(GCC diagnostic pop)
#else
    #define IGNORE_WARNING(wflag)
    #define UNIGNORE_WARNINGS
#endif

#endif
