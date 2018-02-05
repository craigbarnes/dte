#include "load-save.h"
#include "editor.h"
#include "block.h"
#include "wbuf.h"
#include "decoder.h"
#include "encoder.h"
#include "encoding.h"
#include "error.h"
#include "cconv.h"
#include "path.h"

#include <sys/mman.h>

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
    const char *e = detect_encoding_from_bom(buf, size);

    if (b->encoding == NULL) {
        if (e) {
            // UTF-16BE/LE or UTF-32BE/LE
            b->encoding = xstrdup(e);
        }
    } else if (streq(b->encoding, "UTF-16")) {
        // BE or LE?
        if (e && str_has_prefix(e, b->encoding)) {
            free(b->encoding);
            b->encoding = xstrdup(e);
        } else {
            // "open -e UTF-16" but incompatible or no BOM.
            // Do what the user wants. Big-endian is default.
            free(b->encoding);
            b->encoding = xstrdup("UTF-16BE");
        }
    } else if (streq(b->encoding, "UTF-32")) {
        // BE or LE?
        if (e && str_has_prefix(e, b->encoding)) {
            free(b->encoding);
            b->encoding = xstrdup(e);
        } else {
            // "open -e UTF-32" but incompatible or no BOM.
            // Do what the user wants. Big-endian is default.
            free(b->encoding);
            b->encoding = xstrdup("UTF-32BE");
        }
    }

    // Skip BOM only if it matches the specified file encoding.
    if (b->encoding && e && streq(b->encoding, e)) {
        size_t bom_len = 2;
        if (str_has_prefix(e, "UTF-32")) {
            bom_len = 4;
        }
        buf += bom_len;
        size -= bom_len;
    }

    FileDecoder *dec = new_file_decoder(b->encoding, buf, size);
    if (dec == NULL) {
        return -1;
    }

    char *line;
    ssize_t len;
    if (file_decoder_read_line(dec, &line, &len)) {
        Block *blk = NULL;

        if (len && line[len - 1] == '\r') {
            b->newline = NEWLINE_DOS;
            len--;
        }
        blk = add_utf8_line(b, blk, line, len);

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
    if (b->encoding == NULL) {
        e = dec->encoding;
        if (e == NULL) {
            e = editor.charset;
        }
        b->encoding = xstrdup(e);
    }

    free_file_decoder(dec);
    return 0;
}

static int read_blocks(Buffer *b, int fd)
{
    size_t size = b->st.st_size;
    size_t map_size = 64 * 1024;
    unsigned char *buf = NULL;
    bool mapped = false;
    ssize_t rc;

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
        ssize_t alloc = map_size;
        ssize_t pos = 0;

        buf = xnew(char, alloc);
        while (1) {
            rc = xread(fd, buf + pos, alloc - pos);
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
    rc = decode_and_add_blocks(b, buf, size);
    if (mapped) {
        munmap(buf, size);
    } else {
        free(buf);
    }
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
        if (b->st.st_size >= 256LL * 1024LL * 1024LL) {
            error_msg("File exceeds 256MiB file size limit %s", filename);
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

    if (b->encoding == NULL) {
        b->encoding = xstrdup(editor.charset);
    }
    return 0;
}

static char *tmp_filename(const char *filename)
{
    char *dir = path_dirname(filename);
    const char *base = path_basename(filename);
    char *tmp = xsprintf("%s/.tmp.%s.XXXXXX", dir, base);
    free(dir);
    return tmp;
}

static mode_t get_umask(void)
{
    // Wonderful get-and-set API
    mode_t old = umask(0);
    umask(old);
    return old;
}

static int write_buffer(Buffer *b, FileEncoder *enc, const ByteOrderMark *bom)
{
    ssize_t size = 0;
    Block *blk;

    if (bom && !streq(bom->encoding, "UTF-8")) {
        size = bom->len;
        if (xwrite(enc->fd, bom->bytes, size) < 0) {
            goto write_error;
        }
    }
    list_for_each_entry(blk, &b->blocks, node) {
        ssize_t rc = file_encoder_write(enc, blk->data, blk->size);

        if (rc < 0) {
            goto write_error;
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
write_error:
    error_msg("Write error: %s", strerror(errno));
    return -1;
}

int save_buffer (
    Buffer *b,
    const char *filename,
    const char *encoding,
    LineEndingType newline
) {
    FileEncoder *enc;
    char *tmp = NULL;
    int fd;

    // Don't use temporary file when saving file in /tmp because
    // crontab command doesn't like the file to be replaced.
    if (!str_has_prefix(filename, "/tmp/")) {
        // Try to use temporary file first (safer)
        tmp = tmp_filename(filename);
        fd = mkstemp(tmp);
        if (fd < 0) {
            // No write permission to the directory?
            free(tmp);
            tmp = NULL;
        } else if (b->st.st_mode) {
            // Preserve ownership and mode of the original file if possible.

            // "ignoring return value of 'fchown', declared with attribute
            // warn_unused_result"
            //
            // Casting to void does not hide this warning when
            // using GCC and clang does not like this:
            //     int ignore = fchown(...); ignore = ignore;
            if (fchown(fd, b->st.st_uid, b->st.st_gid)) {
            }
            fchmod(fd, b->st.st_mode);
        } else {
            // New file
            fchmod(fd, 0666 & ~get_umask());
        }
    }
    if (tmp == NULL) {
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
    if (write_buffer(b, enc, get_bom_for_encoding(encoding))) {
        close(fd);
        goto error;
    }
    if (close(fd)) {
        error_msg("Close failed: %s", strerror(errno));
        goto error;
    }
    if (tmp != NULL && rename(tmp, filename)) {
        error_msg("Rename failed: %s", strerror(errno));
        goto error;
    }
    free_file_encoder(enc);
    free(tmp);
    stat(filename, &b->st);
    return 0;
error:
    if (enc != NULL) {
        free_file_encoder(enc);
    }
    if (tmp != NULL) {
        unlink(tmp);
        free(tmp);
    } else {
        // Not using temporary file therefore mtime may have changed.
        // Update stat to avoid "File has been modified by someone else"
        // error later when saving the file again.
        stat(filename, &b->st);
    }
    return -1;
}
