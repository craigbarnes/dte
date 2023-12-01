#include "ui.h"
#include "status.h"
#include "util/log.h"

void update_status_line(const Window *window)
{
    EditorState *e = window->editor;
    const GlobalOptions *opts = &e->options;
    const char *lfmt = opts->statusline_left;
    const char *rfmt = opts->statusline_right;
    InputMode mode = e->input_mode;
    char lbuf[512], rbuf[512];
    size_t lw = sf_format(window, opts, mode, lbuf, sizeof lbuf, lfmt);
    size_t rw = sf_format(window, opts, mode, rbuf, sizeof rbuf, rfmt);

    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;
    unsigned int x = window->x;
    unsigned int y = (window->y + window->h) - 1;
    unsigned int w = window->w;
    term_output_reset(term, x, w, 0);
    term_move_cursor(obuf, x, y);
    set_builtin_style(term, &e->styles, BSE_STATUSLINE);

    if (lw <= w) {
        LOG_TRACE("drawing statusline-left; width=%zu", lw);
        term_put_str(obuf, lbuf);
    }

    if (lw <= w && rw <= w && lw + rw > w) {
        // Left and right sides both fit individually, but not together.
        // They'll be drawn overlapping, so there's no inner gap to clear.
        LOG_TRACE("no statusline gap to clear");
    } else {
        if (rw <= w && !term_can_clear_eol_with_el_sequence(term)) {
            // If rbuf will be printed below and EL can't be used, fill the
            // gap between the left and right sides with the minimal number
            // of space characters, instead of letting the fallback path in
            // `term_clear_eol()` print spaces all the way to the end.
            size_t nspaces = w - rw - obuf->x;
            LOG_TRACE("filling statusline gap with %zu spaces", nspaces);
            term_set_bytes(term, ' ', nspaces);
        } else {
            LOG_TRACE("filling statusline gap with term_clear_eol()");
            term_clear_eol(term);
        }
    }

    if (rw <= w) {
        LOG_TRACE("drawing statusline-right; width=%zu", rw);
        obuf->x = w - rw;
        term_move_cursor(obuf, x + w - rw, y);
        term_put_str(obuf, rbuf);
    }
}
