#include "linux.h"
#include "parse.h"
#include "util/xstring.h"

/*
 The Linux console emits \033[[ followed by letters A-E for keys F1-F5.
 This can't be parsed by term_parse_sequence() because [ is a final byte
 (according to ECMA-48) and an input like \033[[A would be parsed as 2
 separate sequences: CSI [ and A. The first of these is meaningless and
 the second corresponds to Shift+A, which is just wrong. Therefore, when
 $TERM is set to "linux", input is first passed through this function to
 handle this special case.
*/
size_t linux_parse_key(const char *buf, size_t length, KeyCode *k)
{
    if (length < 3 || !mem_equal(buf, "\033[[", 3)) {
        return term_parse_sequence(buf, length, k);
    }

    if (unlikely(length == 3)) {
        return TPARSE_PARTIAL_MATCH;
    }

    // Letters A-E represent keys F1-F5
    char c = buf[3];
    if (c >= 'A' && c <= 'E') {
        *k = KEY_F1 + (c - 'A');
        return 4;
    }

    // Consume and ignore "\033[[", as term_parse_sequence() would
    *k = KEY_IGNORE;
    return 3;
}
