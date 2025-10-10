#ifndef UTIL_MACROS_H
#define UTIL_MACROS_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#error C99 compiler required
#endif

#define PASTE(a, b) a##b
#define XPASTE(a, b) PASTE(a, b)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN3(a, b, c) MIN(a, MIN(b, c))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) MAX(a, MAX(b, c))
#define MAX4(a, b, c, d) MAX(a, MAX3(b, c, d))
#define CLAMP(x, lo, hi) MIN(hi, MAX(lo, x))
#define IS_POWER_OF_2(x) (((x) > 0) && (((x) & ((x) - 1)) == 0))
#define DO_PRAGMA(x) _Pragma(#x)

// CHAR_BIT is "required to be 8" since POSIX 2001.
// https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/limits.h.html#tag_14_26_10:~:text=required%20to%20be%208
#define BITSIZE(T) (sizeof(T) * 8)
#define DECIMAL_STR_MAX(T) ((sizeof(T) * 3) + 2)
#define HEX_STR_MAX(T) ((sizeof(T) * 2) + 2)

// Like strlen(3), but suitable for use in constant expressions
// (and only for string literal arguments)
#define STRLEN(x) (sizeof("" x "") - 1)

// Automatically follow a string literal with a comma and its length,
// to avoid the need to manually update both when a string constant
// is changed. Note that this should never be used as an argument of
// library functions (see tools/coccinelle/pitfalls.cocci).
#define STRN(x) x,STRLEN(x)

#define VERSION_GE(L, l, R, r) ((L > R) || ((L == R) && (l >= r)))

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
    #define GNUC_AT_LEAST(x, y) VERSION_GE(__GNUC__, __GNUC_MINOR__, x, y)
#else
    #define GNUC_AT_LEAST(x, y) 0
#endif

#if defined(__clang_major__) && defined(__clang_minor__)
    #define CLANG_AT_LEAST(x, y) VERSION_GE(__clang_major__, __clang_minor__, x, y)
#else
    #define CLANG_AT_LEAST(x, y) 0
#endif

// https://gcc.gnu.org/onlinedocs/gcc-9.5.0/cpp/_005f_005fhas_005fattribute.html
// https://clang.llvm.org/docs/LanguageExtensions.html#has-attribute
#ifdef __has_attribute
    // Supported by GCC 9+ and Clang
    #define HAS_ATTRIBUTE(x) __has_attribute(x)
#else
    #define HAS_ATTRIBUTE(x) 0
#endif

// https://gcc.gnu.org/onlinedocs/gcc-11.4.0/cpp/_005f_005fhas_005fc_005fattribute.html
// https://clang.llvm.org/docs/LanguageExtensions.html#has-c-attribute
#ifdef __has_c_attribute
    // Supported by GCC 11+ and Clang
    #define HAS_C_ATTRIBUTE(x) __has_c_attribute(x)
#else
    #define HAS_C_ATTRIBUTE(x) 0
#endif

// https://gcc.gnu.org/onlinedocs/gcc-10.5.0/cpp/_005f_005fhas_005fbuiltin.html
// https://clang.llvm.org/docs/LanguageExtensions.html#has-builtin
#ifdef __has_builtin
    // Supported by GCC 10+ and Clang
    #define HAS_BUILTIN(x) __has_builtin(x)
#else
    #define HAS_BUILTIN(x) 0
#endif

// https://gcc.gnu.org/onlinedocs/gcc-9.5.0/cpp/_005f_005fhas_005finclude.html
// https://clang.llvm.org/docs/LanguageExtensions.html#has-include
// https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2799.pdf
#ifdef __has_include
    // Supported by GCC 9+ and Clang
    #define HAS_INCLUDE(x) __has_include(x)
#else
    #define HAS_INCLUDE(x) 0
#endif

// https://clang.llvm.org/docs/LanguageExtensions.html#has-warning
#ifdef __has_warning
    // Supported by Clang
    #define HAS_WARNING(x) __has_warning(x)
#else
    #define HAS_WARNING(x) 0
#endif

// https://gcc.gnu.org/onlinedocs/gcc-14.1.0/cpp/_005f_005fhas_005ffeature.html
// https://clang.llvm.org/docs/LanguageExtensions.html#has-feature-and-has-extension
#ifdef __has_feature
    // Supported by GCC 14+ and Clang
    #define HAS_FEATURE(x) __has_feature(x)
#else
    #define HAS_FEATURE(x) 0
#endif

// https://gcc.gnu.org/onlinedocs/gcc-14.1.0/cpp/_005f_005fhas_005fextension.html
// https://clang.llvm.org/docs/LanguageExtensions.html#has-feature-and-has-extension
#ifdef __has_extension
    // __has_extension() is a Clang 3.0+ (and GCC 14+) macro that can
    // be used to check whether a feature is available, even if not
    // standardized in the current -std= mode
    #define HAS_EXTENSION(x) __has_extension(x)
#else
    // Clang versions prior to 3.0 only supported __has_feature()
    #define HAS_EXTENSION(x) HAS_FEATURE(x)
#endif

#if defined(__SANITIZE_ADDRESS__) || HAS_FEATURE(address_sanitizer)
    #define ASAN_ENABLED 1
#else
    #define ASAN_ENABLED 0
#endif

#if defined(__SANITIZE_MEMORY__) || HAS_FEATURE(memory_sanitizer)
    #define MSAN_ENABLED 1
#else
    #define MSAN_ENABLED 0
#endif

#if defined(__TINYC__) && defined(__attribute__)
    // Work around some stupidity in the glibc <sys/cdefs.h> header,
    // so that __attribute__ can be used with TCC.
    // https://lists.nongnu.org/archive/html/tinycc-devel/2018-04/msg00008.html
    #undef __attribute__
#endif

#if __STDC_VERSION__ >= 202311L
    #define UNUSED [[__maybe_unused__]]
#elif GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(unused) || defined(__TINYC__)
    #define UNUSED __attribute__((__unused__))
#else
    #define UNUSED
#endif

#if __STDC_VERSION__ >= 202311L
    #define ALIGNAS(type) alignas(type)
#elif __STDC_VERSION__ >= 201112L
    #define ALIGNAS(type) _Alignas(type)
#elif GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(aligned) || defined(__TINYC__)
    #define ALIGNAS(type) __attribute__((__aligned__(ALIGNOF(type))))
#else
    #define ALIGNAS(type)
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(malloc)
    #define MALLOC WARN_UNUSED_RESULT __attribute__((__malloc__))
#else
    #define MALLOC WARN_UNUSED_RESULT
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(pure)
    #define PURE WARN_UNUSED_RESULT __attribute__((__pure__))
#else
    #define PURE WARN_UNUSED_RESULT
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(const)
    #define CONST_FN WARN_UNUSED_RESULT __attribute__((__const__))
#else
    #define CONST_FN WARN_UNUSED_RESULT
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(constructor) || defined(__TINYC__)
    #define CONSTRUCTOR __attribute__((__constructor__))
#else
    #define CONSTRUCTOR UNUSED
#endif

#if GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(format)
    #define PRINTF(x) __attribute__((__format__(__printf__, (x), (x + 1))))
    #define VPRINTF(x) __attribute__((__format__(__printf__, (x), 0)))
    #define STRFTIME(x) __attribute__((__format__(__strftime__, (x), 0)))
#else
    #define PRINTF(x)
    #define VPRINTF(x)
    #define STRFTIME(x)
#endif

#if (GNUC_AT_LEAST(3, 0) || HAS_BUILTIN(__builtin_expect)) && defined(__OPTIMIZE__)
    #define likely(x) __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
    #define BUILTIN_EXPECT(x, expected) __builtin_expect((x), (expected))
#else
    #define likely(x) (x)
    #define unlikely(x) (x)
    #define BUILTIN_EXPECT(x, expected) (x)
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

#if GNUC_AT_LEAST(3, 1) || HAS_ATTRIBUTE(always_inline) || defined(__TINYC__)
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

#if GNUC_AT_LEAST(5, 0) || HAS_ATTRIBUTE(returns_nonnull)
    #define RETURNS_NONNULL __attribute__((__returns_nonnull__))
#else
    #define RETURNS_NONNULL
#endif

// https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-nonstring-variable-attribute
#if GNUC_AT_LEAST(8, 0) || HAS_ATTRIBUTE(nonstring)
    #define NONSTRING __attribute__((__nonstring__))
#else
    #define NONSTRING
#endif

// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-nonnull_005fif_005fnonzero-function-attribute
#if HAS_ATTRIBUTE(nonnull_if_nonzero)
    #define NONNULL_ARG_IF_NONZERO_LENGTH(arg_idx, len_idx) __attribute__((__nonnull_if_nonzero__(arg_idx, len_idx)))
#else
    #define NONNULL_ARG_IF_NONZERO_LENGTH(arg_idx, len_idx)
#endif

// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-null_005fterminated_005fstring_005farg-function-attribute
#if HAS_ATTRIBUTE(null_terminated_string_arg)
    #define CSTR_ARG(idx) __attribute__((__null_terminated_string_arg__(idx)))
#else
    #define CSTR_ARG(idx)
#endif

// https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-counted_005fby-variable-attribute
// https://clang.llvm.org/docs/AttributeReference.html#counted-by-counted-by-or-null-sized-by-sized-by-or-null
#if HAS_ATTRIBUTE(counted_by)
    // NOTE: DO NOT use this unless the array it's attached to is STRICTLY
    // bounded by the length `member`. A common counter-example of this
    // would be a null-terminated char array, where an extra byte is
    // always appended (and accounted for in allocations) but not included
    // in the length. See commit f0dbfc236381aa1e for more details.
    #define COUNTED_BY(member) __attribute__((counted_by(member)))
#else
    #define COUNTED_BY(member)
#endif

// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-no_005fsanitize-function-attribute
// https://clang.llvm.org/docs/AttributeReference.html#no-sanitize
#if HAS_ATTRIBUTE(no_sanitize)
    #define NO_SANITIZE(...) __attribute__((no_sanitize(__VA_ARGS__)))
#else
    #define NO_SANITIZE(...)
#endif

// https://clang.llvm.org/docs/AttributeReference.html#diagnose-if
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79459
#if HAS_ATTRIBUTE(diagnose_if)
    #define DIAGNOSE_IF(x) __attribute__((diagnose_if((x), (#x), "error")))
#else
    #define DIAGNOSE_IF(x)
#endif

// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-access-function-attribute
#if HAS_ATTRIBUTE(access)
    #define READONLY(...) __attribute__((__access__(read_only, __VA_ARGS__))) // in param
    #define WRITEONLY(...) __attribute__((__access__(write_only, __VA_ARGS__))) // out param
    #define READWRITE(...) __attribute__((__access__(read_write, __VA_ARGS__))) // in-out param
#else
    #define READONLY(...)
    #define WRITEONLY(...)
    #define READWRITE(...)
#endif

#define UNUSED_ARG(x) unused__ ## x UNUSED
#define XMALLOC MALLOC RETURNS_NONNULL
#define XSTRDUP XMALLOC NONNULL_ARGS
#define NONNULL_ARGS_AND_RETURN RETURNS_NONNULL NONNULL_ARGS

#if __STDC_VERSION__ >= 202311L
    #define ALIGNOF(t) alignof(t)
#elif __STDC_VERSION__ >= 201112L
    #define ALIGNOF(t) _Alignof(t)
#elif GNUC_AT_LEAST(3, 0)
    #define ALIGNOF(t) __alignof__(t)
#else
    #define ALIGNOF(t) MIN(sizeof(t), offsetof(struct{char c; t x;}, x))
#endif

#if __STDC_VERSION__ >= 201112L
    #define noreturn _Noreturn
#elif GNUC_AT_LEAST(3, 0) || HAS_ATTRIBUTE(noreturn) || defined(__TINYC__)
    #define noreturn __attribute__((__noreturn__))
#else
    #define noreturn
#endif

#if __STDC_VERSION__ >= 202311L
    // static_assert is a standard keyword in ISO C23 (§6.4.1) and the
    // second argument is optional (§6.7.11)
    #define HAS_STATIC_ASSERT 1
#elif __STDC_VERSION__ >= 201112L
    // ISO C11 provides _Static_assert (§6.4.1), with a mandatory second
    // argument (§6.7.10)
    #define static_assert(x) _Static_assert((x), #x)
    #define HAS_STATIC_ASSERT 1
#elif GNUC_AT_LEAST(4, 6) || HAS_EXTENSION(c_static_assert)
    // GCC 4.6+ and Clang allow _Static_assert (as a language extension),
    // even in pre-C11 standards modes:
    // https://clang.llvm.org/docs/LanguageExtensions.html#c11-static-assert
    #define static_assert(x) __extension__ _Static_assert((x), #x)
    #define HAS_STATIC_ASSERT 1
#else
    #define static_assert(x)
#endif

// https://gcc.gnu.org/onlinedocs/gcc-3.1.1/gcc/Other-Builtins.html#:~:text=__builtin_types_compatible_p
// https://clang.llvm.org/docs/LanguageExtensions.html#builtin-functions:~:text=__builtin_types_compatible_p
#if GNUC_AT_LEAST(3, 1) || HAS_BUILTIN(__builtin_types_compatible_p)
    #define HAS_BUILTIN_TYPES_COMPATIBLE_P 1
#endif

// ARRAYLEN() calculates the number of elements in an array and also
// performs some sanity checks to prevent accidental pointer arguments
#if defined(HAS_STATIC_ASSERT) && defined(HAS_BUILTIN_TYPES_COMPATIBLE_P)
    // https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3369.pdf
    #define SAME_TYPE(a, b) __builtin_types_compatible_p(__typeof__(a), __typeof__(b))
    #define DECAY(a) (&*(a))
    #define IS_ARRAY(a) (!SAME_TYPE(a, DECAY(a)))
    #define MUST_BE(e) (0 * sizeof(struct {_Static_assert(e, #e); int see_n3369;}))
    #define SIZEOF_ARRAY(a) (sizeof(a) + MUST_BE(IS_ARRAY(a)))
    #define ARRAYLEN(a) (SIZEOF_ARRAY(a) / sizeof((a)[0]))
//#elif HAS_EXTENSION(c_countof)
    // https://clang.llvm.org/docs/LanguageExtensions.html#c2y-countof
    // https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3469.htm#:~:text=_Countof
    // https://github.com/llvm/llvm-project/commit/00c43ae23524d72707701620da89ad248393a8e4
    // https://github.com/llvm/llvm-project/issues/102836#issuecomment-2681819243
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117025#c6
    // https://gcc.gnu.org/cgit/gcc/commit/?id=517c9487f8fdc4e4e90252a9365e5823259dc783
    // TODO: Enable this if/when _Countof() becomes more certain and/or available
    // in a Clang release:
    //#define ARRAYLEN(a) _Countof(a)
#else
    // The extra division on the third line produces a `div-by-zero`
    // warning if `a` is a pointer (instead of an array). The GCC/Clang
    // `-Werror=div-by-zero` option is also used by the build system
    // (if available; see `mk/compiler.sh`), so as to turn such warnings
    // into compile-time errors.
    #define ARRAYLEN(a) ( \
        (sizeof(a) / sizeof((a)[0])) \
        / ((size_t)(!(sizeof(a) % sizeof((a)[0])))) \
    )
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

// https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html#index-pragma_002c-diagnostic
// https://clang.llvm.org/docs/UsersManual.html#controlling-diagnostics-via-pragmas
#if GNUC_AT_LEAST(4, 2) || defined(__clang__)
    #define DISABLE_WARNING(wflag) DO_PRAGMA(GCC diagnostic ignored wflag)
#else
    #define DISABLE_WARNING(wflag)
#endif

// https://clang.llvm.org/docs/AttributeReference.html#pragma-clang-loop
// https://gcc.gnu.org/onlinedocs/gcc/Loop-Specific-Pragmas.html#index-pragma-GCC-unroll-n
#if CLANG_AT_LEAST(3, 6)
    #define UNROLL_LOOP(n) DO_PRAGMA(clang loop unroll_count(n))
#elif GNUC_AT_LEAST(8, 0)
    #define UNROLL_LOOP(n) DO_PRAGMA(GCC unroll (n))
#else
    #define UNROLL_LOOP(n)
#endif

// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html (since GCC 4.3)
// https://clang.llvm.org/docs/LanguageExtensions.html#builtin-macros
#ifdef __COUNTER__
    #define COUNTER __COUNTER__
#else
    #define COUNTER __LINE__
#endif

// https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
// https://clang.llvm.org/docs/UsersManual.html#controlling-diagnostics-via-pragmas
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
