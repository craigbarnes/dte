#include "decoder.h"
#include "editor.h"
#include "uchar.h"
#include "common.h"
#include "cconv.h"

static bool fill(FileDecoder *dec)
{
    size_t icount = dec->isize - dec->ipos;

    // Smaller than cconv.obuf to make realloc less likely
    size_t max = 7 * 1024;

    if (icount > max) {
        icount = max;
    }

    if (dec->ipos == dec->isize) {
        return false;
    }

    cconv_process(dec->cconv, dec->ibuf + dec->ipos, icount);
    dec->ipos += icount;
    if (dec->ipos == dec->isize) {
        // Must be flushed after all input has been fed
        cconv_flush(dec->cconv);
    }
    return true;
}

static int set_encoding(FileDecoder *dec, const char *encoding);

static bool detect(FileDecoder *dec, const unsigned char *line, ssize_t len)
{
    for (ssize_t i = 0; i < len; i++) {
        if (line[i] >= 0x80) {
            size_t idx = i;
            unsigned int u = u_get_nonascii(line, len, &idx);
            const char *encoding;

            if (u_is_unicode(u)) {
                encoding = "UTF-8";
            } else if (editor.term_utf8) {
                // UTF-8 terminal, assuming latin1
                encoding = "ISO-8859-1";
            } else {
                // Assuming locale's encoding
                encoding = editor.charset;
            }
            if (set_encoding(dec, encoding)) {
                // FIXME: error message?
                set_encoding(dec, "UTF-8");
            }
            return true;
        }
    }

    // ASCII
    return false;
}

static bool decode_and_read_line(FileDecoder *dec, char **linep, ssize_t *lenp)
{
    char *line;
    ssize_t len;

    while (1) {
        line = cconv_consume_line(dec->cconv, &len);
        if (line) {
            break;
        }

        if (!fill(dec)) {
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

static bool read_utf8_line(FileDecoder *dec, char **linep, ssize_t *lenp)
{
    char *line = (char *)dec->ibuf + dec->ipos;
    const char *nl = memchr(line, '\n', dec->isize - dec->ipos);
    ssize_t len;

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

static bool detect_and_read_line(FileDecoder *dec, char **linep, ssize_t *lenp)
{
    char *line = (char *)dec->ibuf + dec->ipos;
    const char *nl = memchr(line, '\n', dec->isize - dec->ipos);
    ssize_t len;

    if (nl) {
        len = nl - line;
    } else {
        len = dec->isize - dec->ipos;
        if (len == 0) {
            return false;
        }
    }

    if (detect(dec, line, len)) {
        // Encoding detected
        return dec->read_line(dec, linep, lenp);
    }

    // Only ASCII so far
    dec->ipos += len;
    if (nl) {
        dec->ipos++;
    }
    *linep = line;
    *lenp = len;
    return true;
}

static int set_encoding(FileDecoder *dec, const char *encoding)
{
    if (streq(encoding, "UTF-8")) {
        dec->read_line = read_utf8_line;
    } else {
        dec->cconv = cconv_to_utf8(encoding);
        if (dec->cconv == NULL) {
            return -1;
        }
        dec->read_line = decode_and_read_line;
    }
    dec->encoding = xstrdup(encoding);
    return 0;
}

FileDecoder *new_file_decoder (
    const char *encoding,
    const unsigned char *buf,
    ssize_t size
) {
    FileDecoder *dec = xnew0(FileDecoder, 1);

    dec->ibuf = buf;
    dec->isize = size;
    dec->read_line = detect_and_read_line;

    if (encoding) {
        if (set_encoding(dec, encoding)) {
            free_file_decoder(dec);
            return NULL;
        }
    }
    return dec;
}

void free_file_decoder(FileDecoder *dec)
{
    if (dec->cconv != NULL) {
        cconv_free(dec->cconv);
    }
    free(dec->encoding);
    free(dec);
}

bool file_decoder_read_line(FileDecoder *dec, char **linep, ssize_t *lenp)
{
    return dec->read_line(dec, linep, lenp);
}
