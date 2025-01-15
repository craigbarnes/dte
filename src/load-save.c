#include "feature.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "load-save.h"
#include "block.h"
#include "convert.h"
#include "editor.h"
#include "encoding.h"
#include "error.h"
#include "options.h"
#include "util/debug.h"
#include "util/fd.h"
#include "util/list.h"
#include "util/log.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/time-util.h"
#include "util/xreadwrite.h"

static bool decode_and_add_blocks(Buffer *buffer, const unsigned char *buf, size_t size, bool utf8_bom)
{
    EncodingType bom_type = detect_encoding_from_bom(buf, size);
    if (!buffer->encoding && bom_type != UNKNOWN_ENCODING) {
        const char *enc = encoding_from_type(bom_type);
        if (!conversion_supported_by_iconv(enc, "UTF-8")) {
            // TODO: Use error_msg() and return false here?
            LOG_NOTICE("file has %s BOM, but conversion unsupported", enc);
            enc = encoding_from_type(UTF8);
        }
        buffer_set_encoding(buffer, enc, utf8_bom);
    }

    // Skip BOM only if it matches the specified file encoding
    if (bom_type != UNKNOWN_ENCODING && bom_type == lookup_encoding(buffer->encoding)) {
        const ByteOrderMark *bom = get_bom_for_encoding(bom_type);
        if (bom) {
            const size_t bom_len = bom->len;
            buf += bom_len;
            size -= bom_len;
            buffer->bom = true;
        }
    }

    if (!buffer->encoding) {
        buffer->encoding = encoding_from_type(UTF8);
        buffer->bom = utf8_bom;
    }

    return file_decoder_read(buffer, buf, size);
}

static void fixup_blocks(Buffer *buffer)
{
    if (list_empty(&buffer->blocks)) {
        Block *blk = block_new(1);
        list_insert_before(&blk->node, &buffer->blocks);
        return;
    }

    Block *lastblk = BLOCK(buffer->blocks.prev);
    BUG_ON(!lastblk);
    size_t n = lastblk->size;
    if (n && lastblk->data[n - 1] != '\n') {
        // Incomplete lines are not allowed because they're special
        // cases and cause lots of trouble
        block_grow(lastblk, n + 1);
        lastblk->data[n] = '\n';
        lastblk->size++;
        lastblk->nl++;
        buffer->nl++;
    }
}

static int advise_sequential(void *addr, size_t len)
{
    BUG_ON(!addr);
    BUG_ON(len == 0);

#if HAVE_POSIX_MADVISE
    return posix_madvise(addr, len, POSIX_MADV_SEQUENTIAL);
#endif

    // "The posix_madvise() function shall have no effect on the semantics
    // of access to memory in the specified range, although it may affect
    // the performance of access". Ergo, doing nothing is a valid fallback.
    return 0;
}

static bool update_file_info(FileInfo *info, const struct stat *st)
{
    *info = (FileInfo) {
        .size = st->st_size,
        .mode = st->st_mode,
        .gid = st->st_gid,
        .uid = st->st_uid,
        .dev = st->st_dev,
        .ino = st->st_ino,
        .mtime = *get_stat_mtime(st),
    };
    return true;
}

static bool buffer_stat(FileInfo *info, const char *filename)
{
    struct stat st;
    return !stat(filename, &st) && update_file_info(info, &st);
}

static bool buffer_fstat(FileInfo *info, int fd)
{
    struct stat st;
    return !fstat(fd, &st) && update_file_info(info, &st);
}

bool read_blocks(Buffer *buffer, int fd, bool utf8_bom)
{
    const size_t map_size = 64 * 1024;
    size_t size = buffer->file.size;
    unsigned char *buf = NULL;
    bool mapped = false;
    bool ret = false;

    if (size >= map_size) {
        // NOTE: size must be greater than 0
        buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (buf != MAP_FAILED) {
            advise_sequential(buf, size);
            mapped = true;
            goto decode;
        }
        buf = NULL;
    }

    if (likely(size > 0)) {
        buf = malloc(size);
        if (unlikely(!buf)) {
            goto error;
        }
        ssize_t rc = xread_all(fd, buf, size);
        if (unlikely(rc < 0)) {
            goto error;
        }
        size = rc;
    } else {
        // st_size is zero for some files in /proc
        size_t alloc = map_size;
        BUG_ON(!IS_POWER_OF_2(alloc));
        buf = malloc(alloc);
        if (unlikely(!buf)) {
            goto error;
        }
        size_t pos = 0;
        while (1) {
            ssize_t rc = xread_all(fd, buf + pos, alloc - pos);
            if (rc < 0) {
                goto error;
            }
            if (rc == 0) {
                break;
            }
            pos += rc;
            if (pos == alloc) {
                size_t new_alloc = alloc << 1;
                if (unlikely(alloc >= new_alloc)) {
                    errno = EOVERFLOW;
                    goto error;
                }
                alloc = new_alloc;
                char *new_buf = realloc(buf, alloc);
                if (unlikely(!new_buf)) {
                    goto error;
                }
                buf = new_buf;
            }
        }
        size = pos;
    }

decode:
    ret = decode_and_add_blocks(buffer, buf, size, utf8_bom);

error:
    if (mapped) {
        munmap(buf, size);
    } else {
        free(buf);
    }

    if (ret) {
        fixup_blocks(buffer);
    }

    return ret;
}

// Convert size in bytes to size in MiB (rounded up, for any remainder)
static uintmax_t filesize_in_mib(uintmax_t nbytes)
{
    return (nbytes >> 20) + !!(nbytes & 0xFFFFF);
}

UNITTEST {
    const uintmax_t seven_mib = 7UL << 20;
    // NOLINTBEGIN(bugprone-assert-side-effect)
    BUG_ON(filesize_in_mib(seven_mib) != 7);
    BUG_ON(filesize_in_mib(seven_mib - 1) != 7);
    BUG_ON(filesize_in_mib(seven_mib + 1) != 8);
    // NOLINTEND(bugprone-assert-side-effect)
}

bool load_buffer(EditorState *e, Buffer *buffer, const char *filename, bool must_exist)
{
    BUG_ON(buffer->abs_filename);
    BUG_ON(!list_empty(&buffer->blocks));
    const GlobalOptions *gopts = &e->options;

    int fd = xopen(filename, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) {
        if (errno != ENOENT) {
            return error_msg(&e->err, "Error opening %s: %s", filename, strerror(errno));
        }
        if (must_exist) {
            return error_msg(&e->err, "File %s does not exist", filename);
        }
        if (!buffer->encoding) {
            buffer->encoding = encoding_from_type(UTF8);
            buffer->bom = gopts->utf8_bom;
        }
        Block *blk = block_new(1);
        list_insert_before(&blk->node, &buffer->blocks);
        return true;
    }

    FileInfo *info = &buffer->file;
    if (!buffer_fstat(info, fd)) {
        error_msg(&e->err, "fstat failed on %s: %s", filename, strerror(errno));
        goto error;
    }
    if (!S_ISREG(info->mode)) {
        error_msg(&e->err, "Not a regular file %s", filename);
        goto error;
    }

    off_t size = info->size;
    if (unlikely(size < 0)) {
        error_msg(&e->err, "Invalid file size: %jd", (intmax_t)size);
        goto error;
    }

    unsigned int limit_mib = gopts->filesize_limit;
    if (limit_mib > 0) {
        uintmax_t size_mib = filesize_in_mib(size);
        if (unlikely(size_mib > limit_mib)) {
            error_msg (
                &e->err,
                "File size (%juMiB) exceeds 'filesize-limit' option (%uMiB): %s",
                size_mib, limit_mib, filename
            );
            goto error;
        }
    }

    if (!read_blocks(buffer, fd, gopts->utf8_bom)) {
        error_msg(&e->err, "Error reading %s: %s", filename, strerror(errno));
        goto error;
    }

    BUG_ON(!buffer->encoding);
    xclose(fd);
    return true;

error:
    xclose(fd);
    return false;
}

static bool write_buffer (
    Buffer *buffer,
    FileEncoder *enc,
    ErrorBuffer *ebuf,
    const ByteOrderMark *bom,
    int fd
) {
    size_t size = 0;
    if (bom) {
        size = bom->len;
        if (size && xwrite_all(fd, bom->bytes, size) < 0) {
            return error_msg_errno(ebuf, "write");
        }
    }

    const Block *blk;
    block_for_each(blk, &buffer->blocks) {
        ssize_t rc = file_encoder_write(enc, blk->data, blk->size);
        if (rc < 0) {
            return error_msg_errno(ebuf, "write");
        }
        size += rc;
    }

    size_t nr_errors = file_encoder_get_nr_errors(enc);
    if (nr_errors > 0) {
        // Any real error hides this message
        error_msg (
            ebuf,
            "Warning: %zu non-reversible character conversion%s; file saved",
            nr_errors,
            (nr_errors > 1) ? "s" : ""
        );
    }

    // Need to truncate if writing to existing file
    if (xftruncate(fd, size)) {
        return error_msg_errno(ebuf, "ftruncate");
    }

    return true;
}

static int xmkstemp_cloexec(char *path_template)
{
    int fd;
#if HAVE_MKOSTEMP
    do {
        fd = mkostemp(path_template, O_CLOEXEC);
    } while (unlikely(fd == -1 && errno == EINTR));
    return fd;
#endif

    do {
        fd = mkstemp(path_template);
    } while (unlikely(fd == -1 && errno == EINTR));

    if (fd == -1) {
        return fd;
    }

    if (unlikely(!fd_set_cloexec(fd, true))) {
        int e = errno;
        xclose(fd);
        errno = e;
        return -1;
    }

    return fd;
}

static int tmp_file (
    const char *filename,
    const FileInfo *info,
    mode_t new_file_mode,
    char *buf,
    size_t buflen
) {
    if (str_has_prefix(filename, "/tmp/")) {
        // Don't use temporary file when saving file in /tmp because crontab
        // command doesn't like the file to be replaced
        return -1;
    }

    const char *base = path_basename(filename);
    const StringView dir = path_slice_dirname(filename);
    const int dlen = (int)dir.length;
    int n = snprintf(buf, buflen, "%.*s/.tmp.%s.XXXXXX", dlen, dir.data, base);
    if (unlikely(n <= 0 || n >= buflen)) {
        buf[0] = '\0';
        return -1;
    }

    int fd = xmkstemp_cloexec(buf);
    if (fd == -1) {
        // No write permission to the directory?
        buf[0] = '\0';
        return -1;
    }

    if (!info->mode) {
        // New file
        if (xfchmod(fd, new_file_mode) != 0) {
            LOG_WARNING("failed to set file mode: %s", strerror(errno));
        }
        return fd;
    }

    // Preserve ownership and mode of the original file if possible
    if (xfchown(fd, info->uid, info->gid) != 0) {
        LOG_WARNING("failed to preserve file ownership: %s", strerror(errno));
    }
    if (xfchmod(fd, info->mode) != 0) {
        LOG_WARNING("failed to preserve file mode: %s", strerror(errno));
    }

    return fd;
}

static int xfsync(int fd)
{
#if HAVE_FSYNC
    retry:
    if (fsync(fd) == 0) {
        return 0;
    }

    switch (errno) {
    // EINVAL is ignored because it just means "operation not possible
    // on this descriptor" rather than indicating an actual error
    case EINVAL:
    case ENOTSUP:
    case ENOSYS:
        return 0;
    case EINTR:
        goto retry;
    }

    return -1;
#else
    (void)fd;
    return 0;
#endif
}

bool save_buffer(EditorState *e, Buffer *buffer, const char *filename, const FileSaveContext *ctx)
{
    BUG_ON(!ctx->encoding);

    char tmp[8192];
    tmp[0] = '\0';
    int fd = -1;
    if (ctx->hardlinks) {
        LOG_INFO("target file has hard links; writing in-place");
    } else {
        // Try to use temporary file (safer)
        fd = tmp_file(filename, &buffer->file, ctx->new_file_mode, tmp, sizeof(tmp));
    }

    if (fd < 0) {
        // Overwrite the original file directly (if it exists).
        // Ownership is preserved automatically if the file exists.
        mode_t mode = buffer->file.mode;
        if (mode == 0) {
            // New file
            mode = ctx->new_file_mode;
        }
        fd = xopen(filename, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, mode);
        if (fd < 0) {
            return error_msg_errno(&e->err, "open");
        }
    }

    const ByteOrderMark *bom = NULL;
    if (unlikely(ctx->write_bom)) {
        bom = get_bom_for_encoding(lookup_encoding(ctx->encoding));
    }

    FileEncoder enc = file_encoder(ctx->encoding, ctx->crlf, fd);
    if (!write_buffer(buffer, &enc, &e->err, bom, fd)) {
        goto error;
    }

    if (buffer->options.fsync && xfsync(fd) != 0) {
        error_msg_errno(&e->err, "fsync");
        goto error;
    }

    int r = xclose(fd);
    fd = -1;
    if (r != 0) {
        error_msg_errno(&e->err, "close");
        goto error;
    }

    if (tmp[0] && rename(tmp, filename)) {
        error_msg_errno(&e->err, "rename");
        goto error;
    }

    file_encoder_free(&enc);
    buffer_stat(&buffer->file, filename);
    return true;

error:
    xclose(fd);
    file_encoder_free(&enc);

    if (tmp[0]) {
        unlink(tmp);
    } else {
        // Not using temporary file, therefore mtime may have changed.
        // Update stat to avoid "File has been modified by someone else"
        // error later when saving the file again.
        buffer_stat(&buffer->file, filename);
    }

    return false;
}
