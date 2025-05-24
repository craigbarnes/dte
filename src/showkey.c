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
    for (bool loop = true; loop; ) {
        KeyCode key = term_read_input(&term, 100);
        switch (key) {
        case KEY_NONE:
        case KEY_IGNORE:
            continue;
        case KEYCODE_BRACKETED_PASTE:
        case KEYCODE_DETECTED_PASTE:
            term_discard_paste(ibuf, key == KEYCODE_BRACKETED_PASTE);
            continue;
        case MOD_CTRL | 'd':
            loop = false;
        }
        size_t n = copyliteral(buf, "  ");
        n += keycode_to_str(key, buf + n);
        n += copyliteral(buf + n, "\r\n");
        (void)!xwrite_all(STDOUT_FILENO, buf, n);
    }

    term_restore_private_modes(&term);
    term_output_flush(obuf);
    term_cooked();
    return EC_OK;
}
