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
#if HAVE_SIG2STR
    " sig2str"
#endif
#if HAVE_SIGABBREV_NP && !HAVE_SIG2STR
    " sigabbrev_np"
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
