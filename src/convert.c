#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "convert.h"
#include "block.h"
#include "buildvar-iconv.h"
#include "encoding.h"
#include "util/arith.h"
#include "util/debug.h"
#include "util/list.h"
#include "util/log.h"
#include "util/str-util.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

enum {
    // If any line exceeds this length when reading a file, syntax
    // highlighting will be automatically disabled
    SYN_HIGHLIGHT_MAX_LINE_LEN = 512u << 10, // 512KiB
};

typedef struct {
    const unsigned char *ibuf;
    ssize_t ipos;
    ssize_t isize;
    struct cconv *cconv;
} FileDecoder;

static void add_block(Buffer *buffer, Block *blk)
{
    buffer->nl += blk->nl;
    list_insert_before(&blk->node, &buffer->blocks);
}

static Block *add_utf8_line (
    Buffer *buffer,
    Block *blk,
    const unsigned char *line,
    size_t len
) {
    size_t size = len + 1;
    if (blk) {
        size_t avail = blk->alloc - blk->size;
        if (size <= avail) {
            goto copy;
        }
        add_block(buffer, blk);
    }
    size = MAX(size, 8192);
    blk = block_new(size);

copy:
    if (unlikely(len > SYN_HIGHLIGHT_MAX_LINE_LEN && buffer->options.syntax)) {
        // TODO: Make the limit configurable and add documentation
        // TODO: Pass in an ErrorBuffer* and use error_msg() instead of LOG_NOTICE()
        LOG_NOTICE (
            "line length (%zu) exceeded limit (%ju); disabling syntax highlighting",
            len, (uintmax_t)SYN_HIGHLIGHT_MAX_LINE_LEN
        );
        buffer->options.syntax = false;
    }

    memcpy(blk->data + blk->size, line, len);
    blk->size += len;
    blk->data[blk->size++] = '\n';
    blk->nl++;
    return blk;
}

static bool read_utf8_line(FileDecoder *dec, const char **linep, size_t *lenp)
{
    const char *line = dec->ibuf + dec->ipos;
    const char *nl = memchr(line, '\n', dec->isize - dec->ipos);
    size_t len;

    if (nl) {
        len = nl - line;
        dec->ipos += len + 1;
    } else {
        len = dec->isize - dec->ipos;
        if (len == 0) {
            return false;
        }
        dec->ipos += len;
    }

    *linep = line;
    *lenp = len;
    return true;
}

static bool file_decoder_read_utf8(Buffer *buffer, const unsigned char *buf, size_t size)
{
    if (unlikely(!encoding_is_utf8(buffer->encoding))) {
        errno = EINVAL;
        return false;
    }

    FileDecoder dec = {
        .ibuf = buf,
        .isize = size,
    };

    const char *line;
    size_t len;

    if (!read_utf8_line(&dec, &line, &len)) {
        return true;
    }

    if (len && line[len - 1] == '\r') {
        buffer->crlf_newlines = true;
        len--;
    }

    Block *blk = add_utf8_line(buffer, NULL, line, len);

    if (unlikely(buffer->crlf_newlines)) {
        while (read_utf8_line(&dec, &line, &len)) {
            if (len && line[len - 1] == '\r') {
                len--;
            }
            blk = add_utf8_line(buffer, blk, line, len);
        }
    } else {
        while (read_utf8_line(&dec, &line, &len)) {
            blk = add_utf8_line(buffer, blk, line, len);
        }
    }

    if (blk) {
        add_block(buffer, blk);
    }

    return true;
}

static size_t unix_to_dos (
    FileEncoder *enc,
    const unsigned char *buf,
    size_t size
) {
    // TODO: Pass in Buffer::nl and make this size adjustment more conservative
    // (it's resized to handle the worst possible case, despite the fact that we
    // already have the number of newlines pre-computed)
    if (enc->nsize < size * 2) {
        enc->nsize = size * 2;
        enc->nbuf = xrealloc(enc->nbuf, enc->nsize);
    }

    // TODO: Optimize this loop, by making use of memccpy(3)
    size_t d = 0;
    for (size_t s = 0; s < size; s++) {
        unsigned char ch = buf[s];
        if (ch == '\n') {
            enc->nbuf[d++] = '\r';
        }
        enc->nbuf[d++] = ch;
    }

    return d;
}

#if ICONV_DISABLE == 1 // iconv not available; use basic, UTF-8 implementation:

bool conversion_supported_by_iconv (
    const char* UNUSED_ARG(from),
    const char* UNUSED_ARG(to)
) {
    errno = EINVAL;
    return false;
}

FileEncoder file_encoder(const char *encoding, bool crlf, int fd)
{
    if (unlikely(!encoding_is_utf8(encoding))) {
        BUG("unsupported conversion; should have been handled earlier");
    }

    return (FileEncoder) {
        .crlf = crlf,
        .fd = fd,
    };
}

void file_encoder_free(FileEncoder *enc)
{
    free(enc->nbuf);
}

ssize_t file_encoder_write(FileEncoder *enc, const unsigned char *buf, size_t n)
{
    if (unlikely(enc->crlf)) {
        n = unix_to_dos(enc, buf, n);
        buf = enc->nbuf;
    }
    return xwrite_all(enc->fd, buf, n);
}

size_t file_encoder_get_nr_errors(const FileEncoder* UNUSED_ARG(enc))
{
    return 0;
}

bool file_decoder_read(Buffer *buffer, const unsigned char *buf, size_t size)
{
    return file_decoder_read_utf8(buffer, buf, size);
}

#else // ICONV_DISABLE != 1; use full iconv implementation:

#include <iconv.h>

// UTF-8 encoding of U+00BF (inverted question mark; "¿")
#define REPLACEMENT "\xc2\xbf"

struct cconv {
    iconv_t cd;
    char *obuf;
    size_t osize;
    size_t opos;
    size_t consumed;
    size_t errors;

    // Temporary input buffer
    char tbuf[16];
    size_t tcount;

    // REPLACEMENT character, in target encoding
    char rbuf[4];
    size_t rcount;

    // Input character size in bytes, or zero for UTF-8
    size_t char_size;
};

static struct cconv *create(iconv_t cd)
{
    struct cconv *c = xcalloc1(sizeof(*c));
    c->cd = cd;
    c->osize = 8192;
    c->obuf = xmalloc(c->osize);
    return c;
}

static size_t iconv_wrapper (
    iconv_t cd,
    const char **restrict inbuf,
    size_t *restrict inbytesleft,
    char **restrict outbuf,
    size_t *restrict outbytesleft
) {
    // POSIX defines the second parameter of iconv(3) as "char **restrict"
    // but NetBSD declares it as "const char **restrict"
#ifdef __NetBSD__
    const char **restrict in = inbuf;
#else
    char **restrict in = (char **restrict)inbuf;
#endif

    return iconv(cd, in, inbytesleft, outbuf, outbytesleft);
}

static void resize_obuf(struct cconv *c)
{
    c->osize = xmul(2, c->osize);
    c->obuf = xrealloc(c->obuf, c->osize);
}

static void add_replacement(struct cconv *c)
{
    if (c->osize - c->opos < 4) {
        resize_obuf(c);
    }

    memcpy(c->obuf + c->opos, c->rbuf, c->rcount);
    c->opos += c->rcount;
}

static size_t handle_invalid(struct cconv *c, const char *buf, size_t count)
{
    LOG_DEBUG("%zu %zu", c->char_size, count);
    add_replacement(c);
    if (c->char_size == 0) {
        // Converting from UTF-8
        size_t idx = 0;
        CodePoint u = u_get_char(buf, count, &idx);
        LOG_DEBUG("U+%04" PRIX32, u);
        return idx;
    }
    if (c->char_size > count) {
        // wtf
        return 1;
    }
    return c->char_size;
}

static int xiconv(struct cconv *c, const char **ib, size_t *ic)
{
    while (1) {
        char *ob = c->obuf + c->opos;
        size_t oc = c->osize - c->opos;
        size_t rc = iconv_wrapper(c->cd, ib, ic, &ob, &oc);
        c->opos = ob - c->obuf;
        if (rc == (size_t)-1) {
            switch (errno) {
            case EILSEQ:
                c->errors++;
                // Reset
                iconv(c->cd, NULL, NULL, NULL, NULL);
                return errno;
            case EINVAL:
                return errno;
            case E2BIG:
                resize_obuf(c);
                continue;
            default:
                BUG("iconv: %s", strerror(errno));
            }
        } else {
            c->errors += rc;
        }
        return 0;
    }
}

static size_t convert_incomplete(struct cconv *c, const char *input, size_t len)
{
    size_t ipos = 0;
    while (c->tcount < sizeof(c->tbuf) && ipos < len) {
        c->tbuf[c->tcount++] = input[ipos++];
        const char *ib = c->tbuf;
        size_t ic = c->tcount;
        int rc = xiconv(c, &ib, &ic);
        if (ic > 0) {
            memmove(c->tbuf, ib, ic);
        }
        c->tcount = ic;
        if (rc == EINVAL) {
            // Incomplete character at end of input buffer; try again
            // with more input data
            continue;
        }
        if (rc == EILSEQ) {
            // Invalid multibyte sequence
            size_t skip = handle_invalid(c, c->tbuf, c->tcount);
            c->tcount -= skip;
            if (c->tcount > 0) {
                LOG_DEBUG("tcount=%zu, skip=%zu", c->tcount, skip);
                memmove(c->tbuf, c->tbuf + skip, c->tcount);
                continue;
            }
            return ipos;
        }
        break;
    }

    LOG_DEBUG("%zu %zu", ipos, c->tcount);
    return ipos;
}

static void cconv_process(struct cconv *c, const char *input, size_t len)
{
    if (c->consumed > 0) {
        size_t fill = c->opos - c->consumed;
        memmove(c->obuf, c->obuf + c->consumed, fill);
        c->opos = fill;
        c->consumed = 0;
    }

    if (c->tcount > 0) {
        size_t ipos = convert_incomplete(c, input, len);
        input += ipos;
        len -= ipos;
    }

    const char *ib = input;
    for (size_t ic = len; ic > 0; ) {
        int r = xiconv(c, &ib, &ic);
        if (r == EINVAL) {
            // Incomplete character at end of input buffer
            if (ic < sizeof(c->tbuf)) {
                memcpy(c->tbuf, ib, ic);
                c->tcount = ic;
            } else {
                // FIXME
            }
            ic = 0;
            continue;
        }
        if (r == EILSEQ) {
            // Invalid multibyte sequence
            size_t skip = handle_invalid(c, ib, ic);
            ic -= skip;
            ib += skip;
            continue;
        }
    }
}

static struct cconv *cconv_to_utf8(const char *encoding)
{
    iconv_t cd = iconv_open("UTF-8", encoding);
    if (cd == (iconv_t)-1) {
        return NULL;
    }

    struct cconv *c = create(cd);
    c->rcount = copyliteral(c->rbuf, REPLACEMENT);

    if (str_has_prefix(encoding, "UTF-16")) {
        c->char_size = 2;
    } else if (str_has_prefix(encoding, "UTF-32")) {
        c->char_size = 4;
    } else {
        c->char_size = 1;
    }

    return c;
}

static void encode_replacement(struct cconv *c)
{
    static const unsigned char rep[] = REPLACEMENT;
    const char *ib = rep;
    char *ob = c->rbuf;
    size_t ic = STRLEN(REPLACEMENT);
    size_t oc = sizeof(c->rbuf);
    size_t rc = iconv_wrapper(c->cd, &ib, &ic, &ob, &oc);

    if (rc == (size_t)-1) {
        c->rbuf[0] = '\xbf';
        c->rcount = 1;
    } else {
        c->rcount = ob - c->rbuf;
    }
}

static struct cconv *cconv_from_utf8(const char *encoding)
{
    iconv_t cd = iconv_open(encoding, "UTF-8");
    if (cd == (iconv_t)-1) {
        return NULL;
    }
    struct cconv *c = create(cd);
    encode_replacement(c);
    return c;
}

static void cconv_flush(struct cconv *c)
{
    if (c->tcount > 0) {
        // Replace incomplete character at end of input buffer
        LOG_DEBUG("incomplete character at EOF");
        add_replacement(c);
        c->tcount = 0;
    }
}

static char *cconv_consume_line(struct cconv *c, size_t *len)
{
    char *line = c->obuf + c->consumed;
    char *nl = memchr(line, '\n', c->opos - c->consumed);
    if (!nl) {
        *len = 0;
        return NULL;
    }

    size_t n = nl - line + 1;
    c->consumed += n;
    *len = n;
    return line;
}

static char *cconv_consume_all(struct cconv *c, size_t *len)
{
    char *buf = c->obuf + c->consumed;
    *len = c->opos - c->consumed;
    c->consumed = c->opos;
    return buf;
}

static void cconv_free(struct cconv *c)
{
    BUG_ON(!c);
    iconv_close(c->cd);
    free(c->obuf);
    free(c);
}

bool conversion_supported_by_iconv(const char *from, const char *to)
{
    if (unlikely(from[0] == '\0' || to[0] == '\0')) {
        errno = EINVAL;
        return false;
    }

    iconv_t cd = iconv_open(to, from);
    if (cd == (iconv_t)-1) {
        return false;
    }

    iconv_close(cd);
    return true;
}

FileEncoder file_encoder(const char *encoding, bool crlf, int fd)
{
    struct cconv *cconv = NULL;
    if (unlikely(!encoding_is_utf8(encoding))) {
        cconv = cconv_from_utf8(encoding);
        if (!cconv) {
            BUG("unsupported conversion; should have been handled earlier");
        }
    }

    return (FileEncoder) {
        .cconv = cconv,
        .crlf = crlf,
        .fd = fd,
    };
}

void file_encoder_free(FileEncoder *enc)
{
    if (enc->cconv) {
        cconv_free(enc->cconv);
    }
    free(enc->nbuf);
}

// NOTE: buf must contain whole characters!
ssize_t file_encoder_write (
    FileEncoder *enc,
    const unsigned char *buf,
    size_t size
) {
    if (unlikely(enc->crlf)) {
        size = unix_to_dos(enc, buf, size);
        buf = enc->nbuf;
    }
    if (unlikely(enc->cconv)) {
        cconv_process(enc->cconv, buf, size);
        cconv_flush(enc->cconv);
        buf = cconv_consume_all(enc->cconv, &size);
    }
    return xwrite_all(enc->fd, buf, size);
}

size_t file_encoder_get_nr_errors(const FileEncoder *enc)
{
    return enc->cconv ? enc->cconv->errors : 0;
}

static bool fill(FileDecoder *dec)
{
    if (dec->ipos == dec->isize) {
        return false;
    }

    // Smaller than cconv.obuf to make realloc less likely
    size_t max = 7 * 1024;

    size_t icount = MIN(dec->isize - dec->ipos, max);
    cconv_process(dec->cconv, dec->ibuf + dec->ipos, icount);
    dec->ipos += icount;
    if (dec->ipos == dec->isize) {
        // Must be flushed after all input has been fed
        cconv_flush(dec->cconv);
    }
    return true;
}

static bool decode_and_read_line(FileDecoder *dec, const char **linep, size_t *lenp)
{
    char *line;
    size_t len;
    while (1) {
        line = cconv_consume_line(dec->cconv, &len);
        if (line || !fill(dec)) {
            break;
        }
    }

    if (line) {
        // Newline not wanted
        len--;
    } else {
        line = cconv_consume_all(dec->cconv, &len);
        if (len == 0) {
            return false;
        }
    }

    *linep = line;
    *lenp = len;
    return true;
}

bool file_decoder_read(Buffer *buffer, const unsigned char *buf, size_t size)
{
    if (encoding_is_utf8(buffer->encoding)) {
        return file_decoder_read_utf8(buffer, buf, size);
    }

    struct cconv *cconv = cconv_to_utf8(buffer->encoding);
    if (!cconv) {
        return false;
    }

    FileDecoder dec = {
        .ibuf = buf,
        .isize = size,
        .cconv = cconv,
    };

    const char *line;
    size_t len;

    if (decode_and_read_line(&dec, &line, &len)) {
        if (len && line[len - 1] == '\r') {
            buffer->crlf_newlines = true;
            len--;
        }
        Block *blk = add_utf8_line(buffer, NULL, line, len);
        while (decode_and_read_line(&dec, &line, &len)) {
            if (buffer->crlf_newlines && len && line[len - 1] == '\r') {
                len--;
            }
            blk = add_utf8_line(buffer, blk, line, len);
        }
        if (blk) {
            add_block(buffer, blk);
        }
    }

    cconv_free(cconv);
    return true;
}

#endif
