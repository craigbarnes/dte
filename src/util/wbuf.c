#include <string.h>
#include "wbuf.h"
#include "xreadwrite.h"

static size_t wbuf_avail(WriteBuffer *wbuf)
{
    return sizeof(wbuf->buf) - wbuf->fill;
}

bool wbuf_flush(WriteBuffer *wbuf)
{
    if (wbuf->fill) {
        ssize_t rc = xwrite(wbuf->fd, wbuf->buf, wbuf->fill);
        if (unlikely(rc < 0)) {
            return false;
        }
        wbuf->fill = 0;
    }
    return true;
}

bool wbuf_reserve_space(WriteBuffer *wbuf, size_t count)
{
    if (wbuf_avail(wbuf) < count) {
        return wbuf_flush(wbuf);
    }
    return true;
}

bool wbuf_write(WriteBuffer *wbuf, const char *buf, size_t count)
{
    if (count > wbuf_avail(wbuf)) {
        ssize_t rc = wbuf_flush(wbuf);
        if (unlikely(rc < 0)) {
            return false;
        }
        if (unlikely(count >= sizeof(wbuf->buf))) {
            rc = xwrite(wbuf->fd, buf, count);
            if (unlikely(rc < 0)) {
                return false;
            }
            return true;
        }
    }
    memcpy(wbuf->buf + wbuf->fill, buf, count);
    wbuf->fill += count;
    return true;
}

bool wbuf_write_str(WriteBuffer *wbuf, const char *str)
{
    return wbuf_write(wbuf, str, strlen(str));
}

bool wbuf_write_ch(WriteBuffer *wbuf, char ch)
{
    if (wbuf_avail(wbuf) == 0) {
        ssize_t rc = wbuf_flush(wbuf);
        if (unlikely(rc < 0)) {
            return false;
        }
    }
    wbuf->buf[wbuf->fill++] = ch;
    return true;
}
