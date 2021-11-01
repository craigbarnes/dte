#include <stdlib.h>
#include "osc52.h"
#include "util/base64.h"
#include "util/debug.h"
#include "util/macros.h"

bool term_osc52_copy(TermOutputBuffer *output, const char *text, size_t text_len, bool clipboard, bool primary)
{
    BUG_ON(!clipboard && !primary);
    size_t bufsize = (text_len / 3 * 4) + 4;
    char *buf = malloc(bufsize);
    if (unlikely(!buf)) {
        return false;
    }

    term_add_literal(output, "\033]52;");
    if (primary) {
        term_add_byte(output, 'p');
    }
    if (clipboard) {
        term_add_byte(output, 'c');
    }
    term_add_byte(output, ';');

    if (unlikely(text_len == 0)) {
        goto out;
    }

    size_t remainder = text_len % 3;
    size_t ilen = 0;
    size_t olen = 0;
    if (text_len >= 3) {
        ilen = text_len - remainder;
        BUG_ON(ilen == 0);
        BUG_ON(ilen % 3 != 0);
        olen = base64_encode_block(text, ilen, buf, bufsize);
    }

    if (remainder) {
        base64_encode_final(text + ilen, remainder, buf + olen);
        olen += 4;
    }

    term_add_bytes(output, buf, olen);

out:
    free(buf);
    term_add_literal(output, "\033\\");
    return true;
}
