#include "feature.h"
#include "compat.h"

const char feature_string[] =
    ""
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
;
