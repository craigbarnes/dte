#include "feature.h" // HAVE_*
#include <regex.h> // REG_STARTEND, REG_ENHANCED (macOS)
#include <sys/stat.h> // S_ISVTX
#include "buildvar-iconv.h" // ICONV_DISABLE
#include "compat.h"
#include "util/debug.h" // DEBUG
#include "util/macros.h" // ASAN_ENABLED, MSAN_ENABLED
#include "util/unicode.h" // SANE_WCTYPE

const char buildvar_string[] =
    ""
// Features detected via compilation tests (mk/feature-test/*.c)
#if HAVE_DUP3
    " dup3"
#endif
#if HAVE_EMBED
    " embed"
#endif
#if HAVE_PIPE2
    " pipe2"
#endif
#if HAVE_FEXECVE
    " fexecve"
#endif
#if HAVE_FSYNC
    " fsync"
#endif
#if HAVE_MEMFD_CREATE
    " memfd_create"
#endif
#if HAVE_MEMMEM
    " memmem"
#endif
#if HAVE_MEMRCHR
    " memrchr"
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
#if defined(REG_STARTEND) && ASAN_ENABLED == 0 && MSAN_ENABLED == 0
    " REG_STARTEND"
#endif
#ifdef REG_ENHANCED
    " REG_ENHANCED"
#endif
#ifdef S_ISVTX
    " S_ISVTX"
#endif
#if ICONV_DISABLE == 1
    " ICONV_DISABLE"
#endif
#ifdef SANE_WCTYPE
    " SANE_WCTYPE"
#endif
#if ASAN_ENABLED == 1
    " ASan"
#endif
#if MSAN_ENABLED == 1
    " MSan"
#endif

#if DEBUG >= 3
    " DEBUG=3"
#elif DEBUG >= 2
    " DEBUG=2"
#elif DEBUG >= 1
    " DEBUG=1"
#endif
;
