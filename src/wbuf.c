#include "wbuf.h"
#include "common.h"

#include <string.h>

int wbuf_flush(WriteBuffer *wbuf)
{
    if (wbuf->fill) {
        ssize_t rc = xwrite(wbuf->fd, wbuf->buf, wbuf->fill);
        if (rc < 0) {
            return rc;
        }
        wbuf->fill = 0;
    }
    return 0;
}

int wbuf_write(WriteBuffer *wbuf, const char *buf, size_t count)
{
    ssize_t rc;

    if (wbuf->fill + count > sizeof(wbuf->buf)) {
        rc = wbuf_flush(wbuf);
        if (rc < 0) {
            return rc;
        }
    }
    if (count >= sizeof(wbuf->buf)) {
        rc = wbuf_flush(wbuf);
        if (rc < 0) {
            return rc;
        }
        rc = xwrite(wbuf->fd, buf, count);
        if (rc < 0) {
            return rc;
        }
        return 0;
    }
    memcpy(wbuf->buf + wbuf->fill, buf, count);
    wbuf->fill += count;
    return 0;
}

int wbuf_write_str(WriteBuffer *wbuf, const char *str)
{
    return wbuf_write(wbuf, str, strlen(str));
}

int wbuf_write_ch(WriteBuffer *wbuf, char ch)
{
    if (wbuf->fill + 1 > sizeof(wbuf->buf)) {
        ssize_t rc = wbuf_flush(wbuf);
        if (rc < 0) {
            return rc;
        }
    }
    wbuf->buf[wbuf->fill++] = ch;
    return 0;
}
