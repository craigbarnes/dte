#include <stdlib.h>
#include <string.h>
#include "encoder.h"
#include "convert.h"
#include "../util/utf8.h"
#include "../util/xmalloc.h"
#include "../util/xreadwrite.h"

FileEncoder *new_file_encoder(const Encoding *encoding, LineEndingType nls, int fd)
{
    FileEncoder *enc = xnew0(FileEncoder, 1);

    enc->nls = nls;
    enc->fd = fd;

    if (encoding->type == UTF8) {
        enc->cconv = cconv_from_utf8(encoding_to_string(encoding));
        if (enc->cconv == NULL) {
            free(enc);
            return NULL;
        }
    }
    return enc;
}

void free_file_encoder(FileEncoder *enc)
{
    if (enc->cconv != NULL) {
        cconv_free(enc->cconv);
    }
    free(enc->nbuf);
    free(enc);
}

static ssize_t unix_to_dos (
    FileEncoder *enc,
    const unsigned char *buf,
    ssize_t size
) {
    ssize_t s, d;

    if (enc->nsize < size * 2) {
        enc->nsize = size * 2;
        xrenew(enc->nbuf, enc->nsize);
    }

    for (s = 0, d = 0; s < size; s++) {
        unsigned char ch = buf[s];
        if (ch == '\n') {
            enc->nbuf[d++] = '\r';
        }
        enc->nbuf[d++] = ch;
    }
    return d;
}

// NOTE: buf must contain whole characters!
ssize_t file_encoder_write (
    FileEncoder *enc,
    const unsigned char *buf,
    ssize_t size
) {
    if (enc->nls == NEWLINE_DOS) {
        size = unix_to_dos(enc, buf, size);
        buf = enc->nbuf;
    }

    if (enc->cconv == NULL) {
        return xwrite(enc->fd, buf, size);
    }

    cconv_process(enc->cconv, buf, size);
    cconv_flush(enc->cconv);
    buf = cconv_consume_all(enc->cconv, &size);
    return xwrite(enc->fd, buf, size);
}
