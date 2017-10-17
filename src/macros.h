#if defined(__GNUC__)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define NORETURN __attribute__((__noreturn__))
#define FORMAT(idx) __attribute__((format(printf, (idx), (idx + 1))))
#define PURE __attribute__((pure))
#define CONST_FN __attribute__((const))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#define NORETURN
#define FORMAT(idx)
#define PURE
#define CONST_FN
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define static_assert(x) _Static_assert((x), #x)
#else
#define static_assert(x)
#endif

#define ARRAY_COUNT(x) ((unsigned long)sizeof(x) / sizeof(x[0]))
#define memzero(ptr) memset((ptr), 0, sizeof(*(ptr)))
