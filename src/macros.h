#ifndef MACROS_H
#define MACROS_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#error C99 compiler required
#endif

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

#ifdef __has_warning
    #define HAS_WARNING(x) __has_warning(x)
#else
    #define HAS_WARNING(x) 0
#endif

#define DO_PRAGMA(x) _Pragma(#x)

#if GNUC_AT_LEAST(3, 0)
    #define UNUSED(x) UNUSED__ ## x __attribute__((__unused__))
    #define MALLOC __attribute__((__malloc__))
    #define PRINTF(x) __attribute__((__format__(__printf__, (x), (x + 1))))
    #define PURE __attribute__((__pure__))
    #define CONST_FN __attribute__((__const__))
#else
    #define UNUSED
    #define MALLOC
    #define PRINTF(x)
    #define PURE
    #define CONST_FN
#endif

#if GNUC_AT_LEAST(3, 0) && defined(__OPTIMIZE__)
    #define likely(x) __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x) (x)
    #define unlikely(x) (x)
#endif

#if GNUC_AT_LEAST(4, 0) || HAS_ATTRIBUTE(nonnull)
    #define NONNULL_ARGS __attribute__((__nonnull__))
#else
    #define NONNULL_ARGS
#endif

#if GNUC_AT_LEAST(5, 0) || HAS_ATTRIBUTE(returns_nonnull)
    #define RETURNS_NONNULL __attribute__((__returns_nonnull__))
#else
    #define RETURNS_NONNULL
#endif

#if GNUC_AT_LEAST(5, 0)
    #define MESSAGE(x) DO_PRAGMA(message #x)
#else
    #define MESSAGE(x)
#endif

#if __STDC_VERSION__ >= 201112L
    #define NORETURN _Noreturn
#elif GNUC_AT_LEAST(3, 0)
    #define NORETURN __attribute__((__noreturn__))
#else
    #define NORETURN
#endif

#if (__STDC_VERSION__ >= 201112L) || HAS_EXTENSION(c_static_assert)
    #define static_assert(x) _Static_assert((x), #x)
#else
    #define static_assert(x)
#endif

// This macro suppresses warnings about dicarded const qualifiers
// It's only defined for Clang, since GCC is lacking the following
// features required for an acceptable implementation:
//
// 1. The ability to apply pragmas at the granularity of individual
//    function arguments instead of whole statements.
// 2. A flag that only suppresses discarded qualifier warnings without
//    also suppressing incompatible-pointer-types warnings.
// 3. Some mechanism equivalent to __has_warning().
//
#if defined(__clang__) && HAS_WARNING("-Wincompatible-pointer-types-discards-qualifiers")
    #define IGNORE_DISCARDED_QUALIFIERS(x) \
        DO_PRAGMA(clang diagnostic push) \
        DO_PRAGMA(clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers") \
        (x) \
        DO_PRAGMA(clang diagnostic pop)
#else
    #define IGNORE_DISCARDED_QUALIFIERS(x) (x)
#endif

#endif
