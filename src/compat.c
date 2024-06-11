#include "feature.h" // HAVE_*
#include <regex.h> // REG_STARTEND, REG_ENHANCED (macOS)
#include <sys/stat.h> // S_ISVTX
#include "compat.h"
#include "util/macros.h" // ASAN_ENABLED, MSAN_ENABLED
#include "util/unicode.h" // SANE_WCTYPE

// TODO: ICONV_DISABLE

const char feature_string[] =
    ""
// Features detected via compilation tests (mk/feature-test/*.c)
#if HAVE_DUP3
    " dup3"
#endif
#if HAVE_PIPE2
    " pipe2"
#endif
#if HAVE_FSYNC
    " fsync"
#endif
#if HAVE_MEMMEM
    " memmem"
#endif
#if HAVE_QSORT_R
    " qsort_r"
#endif
#if HAVE_MKOSTEMP
    " mkostemp"
#endif
#if HAVE_TIOCGWINSZ
    " TIOCGWINSZ"
#endif
#if HAVE_TCGETWINSIZE && !HAVE_TIOCGWINSZ
    " tcgetwinsize"
#endif
#if HAVE_TIOCNOTTY
    " TIOCNOTTY"
#endif
#if HAVE_POSIX_MADVISE
    " posix_madvise"
#endif

// Features detected via cpp(1) macros
#if defined(REG_STARTEND) && !defined(ASAN_ENABLED) && !defined(MSAN_ENABLED)
    " REG_STARTEND"
#endif
#ifdef REG_ENHANCED
    " REG_ENHANCED"
#endif
#ifdef S_ISVTX
    " S_ISVTX"
#endif
#ifdef SANE_WCTYPE
    " SANE_WCTYPE"
#endif
#ifdef ASAN_ENABLED
    " ASan"
#endif
#ifdef MSAN_ENABLED
    " MSan"
#endif
;
