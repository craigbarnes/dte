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

#define DO_PRAGMA(x) _Pragma(#x)

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define UNUSED(x) UNUSED__ ## x __attribute__((__unused__))
#define NONNULL_ARGS __attribute__((__nonnull__))
#define MALLOC __attribute__((__malloc__))
#define FORMAT(idx) __attribute__((__format__(printf, (idx), (idx + 1))))
#define PURE __attribute__((__pure__))
#define CONST_FN __attribute__((__const__))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#define UNUSED
#define NONNULL_ARGS
#define MALLOC
#define FORMAT(idx)
#define PURE
#define CONST_FN
#endif

#if defined(__GNUC__) && (__GNUC__ >= 5)
#define MESSAGE(x) DO_PRAGMA(message #x)
#define RETURNS_NONNULL __attribute__((__returns_nonnull__))
#else
#define MESSAGE(x)
#define RETURNS_NONNULL
#endif

#if __STDC_VERSION__ >= 201112L
#define NORETURN _Noreturn
#elif defined(__GNUC__)
#define NORETURN __attribute__((__noreturn__))
#else
#define NORETURN
#endif

#if __STDC_VERSION__ >= 201112L
#define static_assert(x) _Static_assert((x), #x)
#else
#define static_assert(x)
#endif

#endif
