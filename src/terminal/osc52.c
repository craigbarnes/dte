#include <stdlib.h>
#include "osc52.h"
#include "output.h"
#include "util/base64.h"
#include "util/debug.h"
#include "util/xstring.h"

bool term_osc52_copy(TermOutputBuffer *output, StringView text, TermCopyFlags flags)
{
    BUG_ON(flags == 0);
    size_t bufsize = (text.length / 3 * 4) + 4;
    char *buf = malloc(bufsize);
    if (unlikely(!buf)) {
        return false;
    }

    size_t plen = !!(flags & TCOPY_PRIMARY);
    size_t clen = !!(flags & TCOPY_CLIPBOARD);
    char *start = term_output_reserve_space(output, 16);
    char *end = xmempcpy4(start, STRN("\033]52;"), "p", plen, "c", clen, STRN(";"));
    output->count += (end - start);

    if (unlikely(text.length == 0)) {
        goto out;
    }

    size_t remainder = text.length % 3;
    size_t ilen = 0;
    size_t olen = 0;
    if (text.length >= 3) {
        ilen = text.length - remainder;
        BUG_ON(ilen == 0);
        BUG_ON(ilen % 3 != 0);
        olen = base64_encode_block(text.data, ilen, buf, bufsize);
    }

    if (remainder) {
        base64_encode_final(text.data + ilen, remainder, buf + olen);
        olen += 4;
    }

    term_put_bytes(output, buf, olen);

out:
    free(buf);
    term_put_literal(output, "\033\\");
    return true;
}
