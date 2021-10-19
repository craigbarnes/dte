#include <errno.h>
#include <stdlib.h>
#include "osc52.h"
#include "output.h"
#include "util/base64.h"
#include "util/debug.h"
#include "util/macros.h"

bool osc52_copy(const char *text, size_t text_len, bool clipboard, bool primary)
{
    BUG_ON(!clipboard && !primary);
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

    size_t bufsize = (text_len / 3 * 4) + 4;
    char *buf = malloc(bufsize);
    if (unlikely(!buf)) {
        return false;
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
    free(buf);

out:
    term_add_literal("\033\\");
    return true;
}

/*
 The kitty OSC 52 implementation can only process chunks of around 6138
 bytes before it fails and breaks out of the OSC 52 input state. The
 documentation (rather dishonestly) tries to sell this as a positive
 thing and claims that its "concat" extension to OSC 52 is somehow
 fixing a natural flaw in the protocol, but in reality it's only fixing
 a flaw in kitty's input handling. Other terminals (e.g. xterm, foot,
 etc.) don't impose such limits and allow sending large amounts of text
 in a single escape sequence.

 - https://sw.kovidgoyal.net/kitty/protocol-extensions.html#pasting-to-clipboard
 - https://github.com/kovidgoyal/kitty/issues/3031
*/
bool kitty_osc52_copy(const char *text, size_t text_len, bool clipboard, bool primary)
{
    // For now we just limit copies to a single 6100 byte chunk.
    // TODO: use kitty's "concat" protocol and remove this limit.
    if (text_len > 6100) {
        errno = EMSGSIZE;
        return false;
    }

    // Kitty requires that the existing contents be cleared first, otherwise
    // the text sent in the second call will be appended to it
    term_add_literal("\033]52;");
    if (primary) {
        term_add_byte('p');
    }
    if (clipboard) {
        term_add_byte('c');
    }
    term_add_literal(";!\033\\");

    if (unlikely(text_len == 0)) {
        return true;
    }

    return osc52_copy(text, text_len, clipboard, primary);
}
