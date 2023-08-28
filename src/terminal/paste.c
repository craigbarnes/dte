#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include "paste.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/string-view.h"
#include "util/xmemmem.h"
#include "util/xreadwrite.h"
#include "util/xstdio.h"

static String term_read_detected_paste(TermInputBuffer *input)
{
    String str = string_new(4096 + input->len);
    if (input->len) {
        string_append_buf(&str, input->buf, input->len);
        input->len = 0;
    }

    while (1) {
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 0
        };

        fd_set set;
        FD_ZERO(&set);

        // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign)
        FD_SET(STDIN_FILENO, &set);

        int rc = select(1, &set, NULL, NULL, &tv);
        if (rc < 0 && errno == EINTR) {
            continue;
        } else if (rc <= 0) {
            break;
        }

        char *buf = string_reserve_space(&str, 4096);
        ssize_t n = xread(STDIN_FILENO, buf, str.alloc - str.len);
        if (n <= 0) {
            break;
        }
        str.len += n;
    }

    string_replace_byte(&str, '\r', '\n');
    const char *plural = (str.len == 1) ? "" : "s";
    LOG_DEBUG("detected paste of %zu byte%s", str.len, plural);
    return str;
}

static String term_read_bracketed_paste(TermInputBuffer *input)
{
    size_t read_max = TERM_INBUF_SIZE;
    String str = string_new(read_max + input->len);
    if (input->len) {
        string_append_buf(&str, input->buf, input->len);
        input->len = 0;
    }

    static const StringView delim = STRING_VIEW("\033[201~");
    char *start, *end;
    ssize_t read_len;

    while (1) {
        start = string_reserve_space(&str, read_max);
        read_len = xread(STDIN_FILENO, start, read_max);
        if (unlikely(read_len <= 0)) {
            goto read_error;
        }
        end = xmemmem(start, read_len, delim.data, delim.length);
        if (end) {
            break;
        }
        str.len += read_len;
    }

    size_t final_chunk_len = (size_t)(end - start);
    str.len += final_chunk_len;
    final_chunk_len += delim.length;
    BUG_ON(final_chunk_len > read_len);

    size_t remainder = read_len - final_chunk_len;
    if (unlikely(remainder)) {
        // Copy anything still in the buffer beyond the end delimiter
        // into the normal input buffer
        LOG_DEBUG("remainder after bracketed paste: %zu", remainder);
        BUG_ON(remainder > TERM_INBUF_SIZE);
        memcpy(input->buf, start + final_chunk_len, remainder);
        input->len = remainder;
    }

    const char *plural = (str.len == 1) ? "" : "s";
    LOG_DEBUG("received bracketed paste of %zu byte%s", str.len, plural);
    string_replace_byte(&str, '\r', '\n');
    return str;

read_error:
    LOG_ERRNO("read");
    string_free(&str);
    BUG_ON(str.buffer);
    return str;
}

String term_read_paste(TermInputBuffer *input, bool bracketed)
{
    if (bracketed) {
        return term_read_bracketed_paste(input);
    }
    return term_read_detected_paste(input);
}

void term_discard_paste(TermInputBuffer *input, bool bracketed)
{
    String str = term_read_paste(input, bracketed);
    string_free(&str);
    LOG_INFO("%spaste discarded", bracketed ? "bracketed " : "");
}
