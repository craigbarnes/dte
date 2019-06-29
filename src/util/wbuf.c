#include <string.h>
#include "macros.h"
#include "wbuf.h"
#include "xreadwrite.h"

#define XFLUSH(buf) do { \
    ssize_t rc_ = wbuf_flush(buf); \
    if (unlikely(rc_ < 0)) { \
        return rc_; \
    } \
} while (0)

NONNULL_ARGS
static size_t wbuf_avail(WriteBuffer *wbuf)
{
    return sizeof(wbuf->buf) - wbuf->fill;
}

ssize_t wbuf_flush(WriteBuffer *wbuf)
{
    if (wbuf->fill) {
        ssize_t rc = xwrite(wbuf->fd, wbuf->buf, wbuf->fill);
        if (unlikely(rc < 0)) {
            return rc;
        }
        wbuf->fill = 0;
    }
    return 0;
}

void wbuf_need_space(WriteBuffer *wbuf, size_t count)
{
    if (wbuf_avail(wbuf) < count) {
        wbuf_flush(wbuf);
    }
}

ssize_t wbuf_write(WriteBuffer *wbuf, const char *buf, size_t count)
{
    if (count > wbuf_avail(wbuf)) {
        XFLUSH(wbuf);
        if (unlikely(count >= sizeof(wbuf->buf))) {
            ssize_t rc = xwrite(wbuf->fd, buf, count);
            if (unlikely(rc < 0)) {
                return rc;
            }
            return 0;
        }
    }
    memcpy(wbuf->buf + wbuf->fill, buf, count);
    wbuf->fill += count;
    return 0;
}

ssize_t wbuf_write_str(WriteBuffer *wbuf, const char *str)
{
    return wbuf_write(wbuf, str, strlen(str));
}

ssize_t wbuf_write_ch(WriteBuffer *wbuf, char ch)
{
    if (wbuf_avail(wbuf) == 0) {
        XFLUSH(wbuf);
    }
    wbuf->buf[wbuf->fill++] = ch;
    return 0;
}
