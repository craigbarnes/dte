#include <errno.h>
#include <string.h>
#include <sys/select.h> // NOLINT(portability-restrict-system-includes)
#include <sys/time.h> // NOLINT(portability-restrict-system-includes)
#include <unistd.h>
#include "paste.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/string-view.h"
#include "util/utf8.h"
#include "util/xmemmem.h"
#include "util/xreadwrite.h"

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

    static const char delim[] = "\033[201~";
    const size_t dlen = sizeof(delim) - 1;
    const char *remainder;
    size_t remainder_len;

    while (1) {
        char *start = string_reserve_space(&str, read_max);
        ssize_t read_len = xread(STDIN_FILENO, start, read_max);
        if (unlikely(read_len <= 0)) {
            goto read_error;
        }

        // Search for the end delimiter, starting from a position that
        // slightly overlaps into the previous read(), so as to handle
        // boundary-straddling delimiters in a minimal amount of code
        size_t overlap = MIN(dlen - 1, str.len);
        unsigned char *end = xmemmem(start - overlap, read_len + overlap, delim, dlen);
        if (!end) {
            str.len += read_len;
            continue;
        }

        size_t total_read_len = str.len + read_len;
        size_t total_text_len = (size_t)(end - str.buffer);
        remainder_len = total_read_len - total_text_len - dlen;
        remainder = end + dlen;
        str.len = total_text_len;
        break;
    }

    if (remainder_len) {
        if (log_level_debug_enabled()) {
            char buf[32];
            u_make_printable(remainder, remainder_len, buf, sizeof(buf), MPF_C0_SYMBOLS);
            LOG_DEBUG("%zu byte remainder after bracketed paste: %s", remainder_len, buf);
        }
        BUG_ON(remainder_len > TERM_INBUF_SIZE);
        memcpy(input->buf, remainder, remainder_len);
        input->len = remainder_len;
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
