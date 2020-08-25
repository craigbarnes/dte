#include "../build/feature.h"
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "load-save.h"
#include "block.h"
#include "convert.h"
#include "editor.h"
#include "encoding.h"
#include "error.h"
#include "util/debug.h"
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

static bool decode_and_add_blocks(Buffer *b, const unsigned char *buf, size_t size)
{
    EncodingType bom_type = detect_encoding_from_bom(buf, size);
    EncodingType enc_type = b->encoding.type;
    if (enc_type == ENCODING_AUTODETECT) {
        if (bom_type != UNKNOWN_ENCODING) {
            BUG_ON(b->encoding.name);
            Encoding e = encoding_from_type(bom_type);
            if (encoding_supported_by_iconv(e.name)) {
                b->encoding = e;
            } else {
                b->encoding = encoding_from_type(UTF8);
            }
        }
    } else if (enc_type == UTF16) {
        b->encoding = encoding_from_type(bom_type == UTF16LE ? UTF16LE : UTF16BE);
    } else if (enc_type == UTF32) {
        b->encoding = encoding_from_type(bom_type == UTF32LE ? UTF32LE : UTF32BE);
    }

    // Skip BOM only if it matches the specified file encoding.
    if (bom_type != UNKNOWN_ENCODING && bom_type == b->encoding.type) {
        const size_t bom_len = get_bom_for_encoding(bom_type)->len;
        buf += bom_len;
        size -= bom_len;
    }

    FileDecoder *dec = new_file_decoder(b->encoding.name, buf, size);
    if (!dec) {
        return false;
    }

    char *line;
    size_t len;
    if (file_decoder_read_line(dec, &line, &len)) {
        if (len && line[len - 1] == '\r') {
            b->crlf_newlines = true;
            len--;
        }
        Block *blk = add_utf8_line(b, NULL, line, len);
        while (file_decoder_read_line(dec, &line, &len)) {
            if (b->crlf_newlines && len && line[len - 1] == '\r') {
                len--;
            }
            blk = add_utf8_line(b, blk, line, len);
        }
        if (blk) {
            add_block(b, blk);
        }
    }

    if (b->encoding.type == ENCODING_AUTODETECT) {
        const char *enc = file_decoder_get_encoding(dec);
        b->encoding = enc ? encoding_from_name(enc) : editor.charset;
    }

    free_file_decoder(dec);
    return true;
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
                blk->alloc = round_size_to_next_multiple(blk->size + 1, 64);
                xrenew(blk->data, blk->alloc);
            }
            blk->data[blk->size++] = '\n';
            blk->nl++;
            b->nl++;
        }
    }
}

static int xmadvise_sequential(void *addr, size_t len)
{
#ifdef HAVE_POSIX_MADVISE
    return posix_madvise(addr, len, POSIX_MADV_SEQUENTIAL);
#else
    // "The posix_madvise() function shall have no effect on the semantics
    // of access to memory in the specified range, although it may affect
    // the performance of access". Ergo, doing nothing is a valid fallback.
    (void)addr;
    (void)len;
    return 0;
#endif
}

bool read_blocks(Buffer *b, int fd)
{
    const size_t map_size = 64 * 1024;
    size_t size = b->file.size;
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
            xmadvise_sequential(buf, size);
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
                return false;
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

    bool r = decode_and_add_blocks(b, buf, size);

    if (mapped) {
        munmap(buf, size);
    } else {
        free(buf);
    }

    if (r) {
        fixup_blocks(b);
    }

    return r;
}

bool load_buffer(Buffer *b, bool must_exist, const char *filename)
{
    int fd = xopen(filename, O_RDONLY | O_CLOEXEC, 0);

    if (fd < 0) {
        if (errno != ENOENT) {
            error_msg("Error opening %s: %s", filename, strerror(errno));
            return false;
        }
        if (must_exist) {
            error_msg("File %s does not exist.", filename);
            return false;
        }
        fixup_blocks(b);
    } else {
        if (!buffer_fstat(b, fd)) {
            error_msg("fstat failed on %s: %s", filename, strerror(errno));
            xclose(fd);
            return false;
        }
        if (!S_ISREG(b->file.mode)) {
            error_msg("Not a regular file %s", filename);
            xclose(fd);
            return false;
        }
        if (b->file.size / 1024 / 1024 > editor.options.filesize_limit) {
            error_msg (
                "File size exceeds 'filesize-limit' option (%uMiB): %s",
                editor.options.filesize_limit,
                filename
            );
            xclose(fd);
            return false;
        }

        if (!read_blocks(b, fd)) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
            xclose(fd);
            return false;
        }
        xclose(fd);
    }

    if (b->encoding.type == ENCODING_AUTODETECT) {
        b->encoding = editor.charset;
    }

    return true;
}

static mode_t get_umask(void)
{
    // Wonderful get-and-set API
    mode_t old = umask(0);
    umask(old);
    return old;
}

static bool write_buffer(Buffer *b, FileEncoder *enc, int fd, EncodingType bom_type)
{
    size_t size = 0;
    if (bom_type != UTF8) {
        const ByteOrderMark *bom = get_bom_for_encoding(bom_type);
        if (bom) {
            size = bom->len;
            if (xwrite(fd, bom->bytes, size) < 0) {
                perror_msg("write");
                return false;
            }
        }
    }

    Block *blk;
    block_for_each(blk, &b->blocks) {
        ssize_t rc = file_encoder_write(enc, blk->data, blk->size);
        if (rc < 0) {
            perror_msg("write");
            return false;
        }
        size += rc;
    }

    size_t nr_errors = file_encoder_get_nr_errors(enc);
    if (nr_errors > 0) {
        // Any real error hides this message
        error_msg (
            "Warning: %zu nonreversible character conversions. File saved.",
            nr_errors
        );
    }

    // Need to truncate if writing to existing file
    if (ftruncate(fd, size)) {
        perror_msg("ftruncate");
        return false;
    }

    return true;
}

bool save_buffer (
    Buffer *b,
    const char *filename,
    const Encoding *encoding,
    bool crlf
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
        } else if (b->file.mode) {
            // Preserve ownership and mode of the original file if possible.
            UNUSED int u1 = fchown(fd, b->file.uid, b->file.gid);
            UNUSED int u2 = fchmod(fd, b->file.mode);
        } else {
            // New file
            fchmod(fd, 0666 & ~get_umask());
        }
    }

    if (fd < 0) {
        // Overwrite the original file (if exists) directly.
        // Ownership is preserved automatically if the file exists.
        mode_t mode = b->file.mode;
        if (mode == 0) {
            // New file.
            mode = 0666 & ~get_umask();
        }
        fd = xopen(filename, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, mode);
        if (fd < 0) {
            perror_msg("open");
            return false;
        }
    }

    enc = new_file_encoder(encoding, crlf, fd);
    if (unlikely(!enc)) {
        // This should never happen because encoding is validated early
        perror_msg("new_file_encoder");
        xclose(fd);
        goto error;
    }
    if (!write_buffer(b, enc, fd, encoding->type)) {
        xclose(fd);
        goto error;
    }

#ifdef HAVE_FSYNC
    if (b->options.fsync) {
        retry:
        if (fsync(fd) != 0) {
            switch (errno) {
            // EINVAL is ignored because it just means "operation not
            // possible on this descriptor" rather than indicating an
            // actual error
            case EINVAL:
            case ENOTSUP:
            case ENOSYS:
                break;
            case EINTR:
                goto retry;
            default:
                perror_msg("fsync");
                xclose(fd);
                goto error;
            }
        }
    }
#endif

    if (xclose(fd)) {
        perror_msg("close");
        goto error;
    }
    if (*tmp && rename(tmp, filename)) {
        perror_msg("rename");
        goto error;
    }
    free_file_encoder(enc);
    buffer_stat(b, filename);
    return true;

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
        buffer_stat(b, filename);
    }
    return false;
}
