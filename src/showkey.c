#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "showkey.h"
#include "terminal/input.h"
#include "terminal/key.h"
#include "terminal/mode.h"
#include "terminal/output.h"
#include "terminal/paste.h"
#include "terminal/terminal.h"
#include "util/str-util.h"
#include "util/xreadwrite.h"

static bool is_ignored_key(KeyCode key, TermInputBuffer *ibuf)
{
    bool bpaste = (key == KEYCODE_BRACKETED_PASTE);
    if (bpaste || key == KEYCODE_DETECTED_PASTE) {
        term_discard_paste(ibuf, bpaste);
        return true;
    }
    return key == KEY_NONE || key == KEY_IGNORE;
}

ExitCode showkey_loop(unsigned int terminal_query_level)
{
    if (!term_mode_init()) {
        return ec_error("tcgetattr", EC_IO_ERROR);
    }
    if (unlikely(!term_raw())) {
        return ec_error("tcsetattr", EC_IO_ERROR);
    }

    Terminal term = {.obuf = TERM_OUTPUT_INIT};
    TermOutputBuffer *obuf = &term.obuf;
    TermInputBuffer *ibuf = &term.ibuf;
    term_init(&term, getenv("TERM"), getenv("COLORTERM"));
    term_enable_private_modes(&term);
    term_put_initial_queries(&term, terminal_query_level);
    term_put_literal(obuf, "Press any key combination, or use Ctrl+D to exit\r\n");
    term_output_flush(obuf);

    char buf[KEYCODE_STR_BUFSIZE + STRLEN("  \r\n")];
    size_t prefix_len = copyliteral(buf, "  ");

    for (KeyCode key = KEY_NONE; key != (MOD_CTRL | 'd'); ) {
        key = term_read_input(&term, 100);
        if (is_ignored_key(key, ibuf)) {
            continue;
        }

        size_t n = prefix_len;
        n += keycode_to_str(key, buf + n);
        n += copyliteral(buf + n, "\r\n");
        (void)!xwrite_all(STDOUT_FILENO, buf, n);
    }

    term_restore_private_modes(&term);
    term_output_flush(obuf);
    term_cooked();
    return EC_OK;
}
