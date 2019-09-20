#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "load-save.h"
#include "block.h"
#include "debug.h"
#include "editor.h"
#include "encoding/bom.h"
#include "encoding/convert.h"
#include "encoding/decoder.h"
#include "encoding/encoder.h"
#include "error.h"
#include "util/macros.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "util/xsnprintf.h"

static void add_block(Buffer *b, Block *blk)
{
    b->nl += blk->nl;
    list_add_before(&blk->node, &b->blocks);
}

static Block *add_utf8_line (
    Buffer *b,
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
        add_block(b, blk);
    }
    if (size < 8192) {
        size = 8192;
    }
    blk = block_new(size);
copy:
    memcpy(blk->data + blk->size, line, len);
    blk->size += len;
    blk->data[blk->size++] = '\n';
    blk->nl++;
    return blk;
}

static int decode_and_add_blocks (
    Buffer *b,
    const unsigned char *buf,
    size_t size
) {
    EncodingType bom_type = detect_encoding_from_bom(buf, size);

    switch (b->encoding.type) {
    case ENCODING_AUTODETECT:
        if (bom_type != UNKNOWN_ENCODING) {
            BUG_ON(b->encoding.name != NULL);
            Encoding e = encoding_from_type(bom_type);
            if (encoding_supported_by_iconv(e.name)) {
                b->encoding = e;
            } else {
                b->encoding = encoding_from_type(UTF8);
            }
        }
        break;
    case UTF16:
        switch (bom_type) {
        case UTF16LE:
        case UTF16BE:
            b->encoding = encoding_from_type(bom_type);
            break;
        default:
            // "open -e UTF-16" but incompatible or no BOM.
            // Do what the user wants. Big-endian is default.
            b->encoding = encoding_from_type(UTF16BE);
        }
        break;
    case UTF32:
        switch (bom_type) {
        case UTF32LE:
        case UTF32BE:
            b->encoding = encoding_from_type(bom_type);
            break;
        default:
            // "open -e UTF-32" but incompatible or no BOM.
            // Do what the user wants. Big-endian is default.
            b->encoding = encoding_from_type(UTF32BE);
        }
        break;
    default:
        break;
    }

    // Skip BOM only if it matches the specified file encoding.
    if (bom_type != UNKNOWN_ENCODING && bom_type == b->encoding.type) {
        const size_t bom_len = get_bom_for_encoding(bom_type)->len;
        buf += bom_len;
        size -= bom_len;
    }

    FileDecoder *dec = new_file_decoder(b->encoding.name, buf, size);
    if (dec == NULL) {
        return -1;
    }

    char *line;
    size_t len;
    if (file_decoder_read_line(dec, &line, &len)) {
        if (len && line[len - 1] == '\r') {
            b->newline = NEWLINE_DOS;
            len--;
        }
        Block *blk = add_utf8_line(b, NULL, line, len);
        while (file_decoder_read_line(dec, &line, &len)) {
            if (b->newline == NEWLINE_DOS && len && line[len - 1] == '\r') {
                len--;
            }
            blk = add_utf8_line(b, blk, line, len);
        }
        if (blk) {
            add_block(b, blk);
        }
    }

    if (b->encoding.type == ENCODING_AUTODETECT) {
        if (dec->encoding) {
            b->encoding = encoding_from_name(dec->encoding);
        } else {
            b->encoding = editor.charset;
        }
    }

    free_file_decoder(dec);
    return 0;
}

static void fixup_blocks(Buffer *b)
{
    if (list_empty(&b->blocks)) {
        Block *blk = block_new(1);
        list_add_before(&blk->node, &b->blocks);
    } else {
        // Incomplete lines are not allowed because they are
        // special cases and cause lots of trouble.
        Block *blk = BLOCK(b->blocks.prev);
        if (blk->size && blk->data[blk->size - 1] != '\n') {
            if (blk->size == blk->alloc) {
                blk->alloc = ROUND_UP(blk->size + 1, 64);
                xrenew(blk->data, blk->alloc);
            }
            blk->data[blk->size++] = '\n';
            blk->nl++;
            b->nl++;
        }
    }
}

int read_blocks(Buffer *b, int fd)
{
    size_t size = b->st.st_size;
    size_t map_size = 64 * 1024;
    unsigned char *buf = NULL;
    bool mapped = false;

    // st_size is zero for some files in /proc.
    // Can't mmap files in /proc and /sys.
    if (size >= map_size) {
        // NOTE: size must be greater than 0
        buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (buf == MAP_FAILED) {
            buf = NULL;
        } else {
            mapped = true;
        }
    }

    if (!mapped) {
        size_t alloc = map_size;
        size_t pos = 0;

        buf = xmalloc(alloc);
        while (1) {
            ssize_t rc = xread(fd, buf + pos, alloc - pos);
            if (rc < 0) {
                free(buf);
                return -1;
            }
            if (rc == 0) {
                break;
            }
            pos += rc;
            if (pos == alloc) {
                alloc *= 2;
                xrenew(buf, alloc);
            }
        }
        size = pos;
    }

    int rc = decode_and_add_blocks(b, buf, size);

    if (mapped) {
        munmap(buf, size);
    } else {
        free(buf);
    }

    if (rc) {
        return rc;
    }

    fixup_blocks(b);
    return rc;
}

int load_buffer(Buffer *b, bool must_exist, const char *filename)
{
    int fd = open(filename, O_RDONLY);

    if (fd < 0) {
        if (errno != ENOENT) {
            error_msg("Error opening %s: %s", filename, strerror(errno));
            return -1;
        }
        if (must_exist) {
            error_msg("File %s does not exist.", filename);
            return -1;
        }
        fixup_blocks(b);
    } else {
        if (fstat(fd, &b->st) != 0) {
            error_msg("fstat failed on %s: %s", filename, strerror(errno));
            close(fd);
            return -1;
        }
        if (!S_ISREG(b->st.st_mode)) {
            error_msg("Not a regular file %s", filename);
            close(fd);
            return -1;
        }
        if (b->st.st_size / 1024 / 1024 > editor.options.filesize_limit) {
            error_msg (
                "File size exceeds 'filesize-limit' option (%uMiB): %s",
                editor.options.filesize_limit,
                filename
            );
            close(fd);
            return -1;
        }

        if (read_blocks(b, fd)) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (b->encoding.type == ENCODING_AUTODETECT) {
        b->encoding = editor.charset;
    }

    return 0;
}

static mode_t get_umask(void)
{
    // Wonderful get-and-set API
    mode_t old = umask(0);
    umask(old);
    return old;
}

static int write_buffer(Buffer *b, FileEncoder *enc, EncodingType bom_type)
{
    size_t size = 0;
    if (bom_type != UTF8) {
        const ByteOrderMark *bom = get_bom_for_encoding(bom_type);
        if (bom) {
            size = bom->len;
            if (xwrite(enc->fd, bom->bytes, size) < 0) {
                error_msg("Write error: %s", strerror(errno));
                return -1;
            }
        }
    }

    Block *blk;
    block_for_each(blk, &b->blocks) {
        ssize_t rc = file_encoder_write(enc, blk->data, blk->size);
        if (rc < 0) {
            error_msg("Write error: %s", strerror(errno));
            return -1;
        }
        size += rc;
    }

    if (enc->cconv != NULL && cconv_nr_errors(enc->cconv)) {
        // Any real error hides this message
        error_msg (
            "Warning: %zu nonreversible character conversions. File saved.",
            cconv_nr_errors(enc->cconv)
        );
    }

    // Need to truncate if writing to existing file
    if (ftruncate(enc->fd, size)) {
        error_msg("Truncate failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int save_buffer (
    Buffer *b,
    const char *filename,
    const Encoding *encoding,
    LineEndingType newline
) {
    FileEncoder *enc;
    int fd = -1;
    char tmp[8192];
    tmp[0] = '\0';

    // Don't use temporary file when saving file in /tmp because
    // crontab command doesn't like the file to be replaced.
    if (!str_has_prefix(filename, "/tmp/")) {
        // Try to use temporary file first (safer)
        const char *base = path_basename(filename);
        const StringView dir = path_slice_dirname(filename);
        const int dlen = (int)dir.length;
        xsnprintf(tmp, sizeof tmp, "%.*s/.tmp.%s.XXXXXX", dlen, dir.data, base);
        fd = mkstemp(tmp);
        if (fd < 0) {
            // No write permission to the directory?
            tmp[0] = '\0';
        } else if (b->st.st_mode) {
            // Preserve ownership and mode of the original file if possible.
            UNUSED int u1 = fchown(fd, b->st.st_uid, b->st.st_gid);
            UNUSED int u2 = fchmod(fd, b->st.st_mode);
        } else {
            // New file
            fchmod(fd, 0666 & ~get_umask());
        }
    }

    if (fd < 0) {
        // Overwrite the original file (if exists) directly.
        // Ownership is preserved automatically if the file exists.
        mode_t mode = b->st.st_mode;
        if (mode == 0) {
            // New file.
            mode = 0666 & ~get_umask();
        }
        fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, mode);
        if (fd < 0) {
            error_msg("Error opening file: %s", strerror(errno));
            return -1;
        }
    }

    enc = new_file_encoder(encoding, newline, fd);
    if (enc == NULL) {
        // This should never happen because encoding is validated early
        error_msg("iconv_open: %s", strerror(errno));
        close(fd);
        goto error;
    }
    if (write_buffer(b, enc, encoding->type)) {
        close(fd);
        goto error;
    }
    if (close(fd)) {
        error_msg("Close failed: %s", strerror(errno));
        goto error;
    }
    if (*tmp && rename(tmp, filename)) {
        error_msg("Rename failed: %s", strerror(errno));
        goto error;
    }
    free_file_encoder(enc);
    stat(filename, &b->st);
    return 0;
error:
    if (enc != NULL) {
        free_file_encoder(enc);
    }
    if (*tmp) {
        unlink(tmp);
    } else {
        // Not using temporary file therefore mtime may have changed.
        // Update stat to avoid "File has been modified by someone else"
        // error later when saving the file again.
        stat(filename, &b->st);
    }
    return -1;
}
