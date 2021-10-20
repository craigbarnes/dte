#include <stdlib.h>
#include "osc52.h"
#include "output.h"
#include "util/base64.h"
#include "util/debug.h"
#include "util/macros.h"

bool osc52_copy(const char *text, size_t text_len, bool clipboard, bool primary)
{
    BUG_ON(!clipboard && !primary);
    size_t bufsize = (text_len / 3 * 4) + 4;
    char *buf = malloc(bufsize);
    if (unlikely(!buf)) {
        return false;
    }

    term_add_literal("\033]52;");
    if (primary) {
        term_add_byte('p');
    }
    if (clipboard) {
        term_add_byte('c');
    }
    term_add_byte(';');

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

    term_add_bytes(buf, olen);

out:
    free(buf);
    term_add_literal("\033\\");
    return true;
}
