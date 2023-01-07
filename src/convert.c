#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "convert.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/log.h"
#include "util/str-util.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

struct FileEncoder {
    struct cconv *cconv;
    unsigned char *nbuf;
    size_t nsize;
    bool crlf;
    int fd;
};

struct FileDecoder {
    const char *encoding;
    const unsigned char *ibuf;
    ssize_t ipos, isize;
    struct cconv *cconv;
    bool (*read_line)(struct FileDecoder *dec, const char **linep, size_t *lenp);
};

const char *file_decoder_get_encoding(const FileDecoder *dec)
{
    return dec->encoding;
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

static size_t unix_to_dos (
    FileEncoder *enc,
    const unsigned char *buf,
    size_t size
) {
    if (enc->nsize < size * 2) {
        enc->nsize = size * 2;
        xrenew(enc->nbuf, enc->nsize);
    }
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

#ifdef ICONV_DISABLE // iconv not available; use basic, UTF-8 implementation:

bool conversion_supported_by_iconv (
    const char* UNUSED_ARG(from),
    const char* UNUSED_ARG(to)
) {
    errno = EINVAL;
    return false;
}

FileEncoder *new_file_encoder(const Encoding *encoding, bool crlf, int fd)
{
    if (unlikely(encoding->type != UTF8)) {
        errno = EINVAL;
        return NULL;
    }
    FileEncoder *enc = xnew0(FileEncoder, 1);
    enc->crlf = crlf;
    enc->fd = fd;
    return enc;
}

void free_file_encoder(FileEncoder *enc)
{
    free(enc->nbuf);
    free(enc);
}

ssize_t file_encoder_write(FileEncoder *enc, const unsigned char *buf, size_t n)
{
    if (enc->crlf) {
        n = unix_to_dos(enc, buf, n);
        buf = enc->nbuf;
    }
    return xwrite_all(enc->fd, buf, n);
}

size_t file_encoder_get_nr_errors(const FileEncoder* UNUSED_ARG(enc))
{
    return 0;
}

FileDecoder *new_file_decoder(const char *encoding, const unsigned char *buf, size_t n)
{
    if (unlikely(encoding && !streq(encoding, "UTF-8"))) {
        errno = EINVAL;
        return NULL;
    }
    FileDecoder *dec = xnew0(FileDecoder, 1);
    dec->ibuf = buf;
    dec->isize = n;
    return dec;
}

void free_file_decoder(FileDecoder *dec)
{
    free(dec);
}

bool file_decoder_read_line(FileDecoder *dec, const char **linep, size_t *lenp)
{
    return read_utf8_line(dec, linep, lenp);
}

#else // ICONV_DISABLE is undefined; use full iconv implementation:

#include <iconv.h>

static const unsigned char replacement[2] = "\xc2\xbf"; // U+00BF

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

    // Replacement character 0xBF (inverted question mark)
    char rbuf[4];
    size_t rcount;

    // Input character size in bytes, or zero for UTF-8
    size_t char_size;
};

static struct cconv *create(iconv_t cd)
{
    struct cconv *c = xnew0(struct cconv, 1);
    c->cd = cd;
    c->osize = 8192;
    c->obuf = xmalloc(c->osize);
    return c;
}

static size_t encoding_char_size(const char *encoding)
{
    if (str_has_prefix(encoding, "UTF-16")) {
        return 2;
    }
    if (str_has_prefix(encoding, "UTF-32")) {
        return 4;
    }
    return 1;
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

static void encode_replacement(struct cconv *c)
{
    const char *ib = replacement;
    char *ob = c->rbuf;
    size_t ic = sizeof(replacement);
    size_t oc = sizeof(c->rbuf);
    size_t rc = iconv_wrapper(c->cd, &ib, &ic, &ob, &oc);

    if (rc == (size_t)-1) {
        c->rbuf[0] = '\xbf';
        c->rcount = 1;
    } else {
        c->rcount = ob - c->rbuf;
    }
}

static void resize_obuf(struct cconv *c)
{
    c->osize *= 2;
    xrenew(c->obuf, c->osize);
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
    memcpy(c->rbuf, replacement, sizeof(replacement));
    c->rcount = sizeof(replacement);
    c->char_size = encoding_char_size(encoding);
    return c;
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

FileEncoder *new_file_encoder(const Encoding *encoding, bool crlf, int fd)
{
    FileEncoder *enc = xnew0(FileEncoder, 1);
    enc->crlf = crlf;
    enc->fd = fd;

    if (encoding->type != UTF8) {
        enc->cconv = cconv_from_utf8(encoding->name);
        if (!enc->cconv) {
            free(enc);
            return NULL;
        }
    }

    return enc;
}

void free_file_encoder(FileEncoder *enc)
{
    if (enc->cconv) {
        cconv_free(enc->cconv);
    }
    free(enc->nbuf);
    free(enc);
}

// NOTE: buf must contain whole characters!
ssize_t file_encoder_write (
    FileEncoder *enc,
    const unsigned char *buf,
    size_t size
) {
    if (enc->crlf) {
        size = unix_to_dos(enc, buf, size);
        buf = enc->nbuf;
    }
    if (enc->cconv) {
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

static bool set_encoding(FileDecoder *dec, const char *encoding)
{
    if (strcmp(encoding, "UTF-8") == 0) {
        dec->read_line = read_utf8_line;
    } else {
        dec->cconv = cconv_to_utf8(encoding);
        if (!dec->cconv) {
            return false;
        }
        dec->read_line = decode_and_read_line;
    }
    dec->encoding = str_intern(encoding);
    return true;
}

FileDecoder *new_file_decoder (
    const char *encoding,
    const unsigned char *buf,
    size_t size
) {
    FileDecoder *dec = xnew0(FileDecoder, 1);
    dec->ibuf = buf;
    dec->isize = size;

    if (!encoding) {
        encoding = "UTF-8";
    }

    if (!set_encoding(dec, encoding)) {
        free_file_decoder(dec);
        return NULL;
    }

    return dec;
}

void free_file_decoder(FileDecoder *dec)
{
    if (dec->cconv) {
        cconv_free(dec->cconv);
    }
    free(dec);
}

bool file_decoder_read_line(FileDecoder *dec, const char **linep, size_t *lenp)
{
    return dec->read_line(dec, linep, lenp);
}

#endif
