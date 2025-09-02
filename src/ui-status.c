#include "ui.h"
#include "editor.h"
#include "status.h"
#include "trace.h"

void update_status_line(const Window *window)
{
    EditorState *e = window->editor;
    const GlobalOptions *opts = &e->options;
    const char *lfmt = opts->statusline_left;
    const char *rfmt = opts->statusline_right;
    const ModeHandler *mode = e->mode;
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
        term_put_str(obuf, lbuf);
    }

    const char *draw_sides;
    const char *gap_method;
    int gap_width;

    if (lw <= w && rw <= w && lw + rw > w) {
        // Left and right sides both fit individually, but not together.
        // They'll be drawn overlapping, so there's no inner gap to clear.
        // gap_width is negative and set only for logging purposes.
        draw_sides = "LR";
        gap_method = "overlap";
        gap_width = w - (lw + rw);
    } else {
        draw_sides = (rw <= w) ? (lw <= w ? "LR" : "R") : "L";
        if (rw <= w && !term_can_clear_eol_with_el_sequence(term)) {
            // If rbuf will be printed below and EL can't be used, fill the
            // gap between the left and right sides with the minimal number
            // of space characters, instead of letting the fallback path in
            // `term_clear_eol()` print spaces all the way to the end.
            gap_width = w - rw - obuf->x;
            TermSetBytesMethod m = term_set_bytes(term, ' ', gap_width);
            gap_method = (m == TERM_SET_BYTES_REP) ? "REP" : "spaces";
        } else {
            int n = term_clear_eol(term);
            bool el = (n == TERM_CLEAR_EOL_USED_EL);
            gap_method = (n >= 0) ? "spaces" : (el ? "EL" : "REP");
            if (lw > w && rw > w) {
                draw_sides = "none";
                gap_width = w;
            } else {
                gap_width = w - (rw <= w ? rw : lw);
            }
        }
    }

    if (rw <= w) {
        obuf->x = w - rw;
        term_move_cursor(obuf, x + w - rw, y);
        term_put_str(obuf, rbuf);
    }

    TRACE_OUTPUT (
        "drawing statusline: left=%zu right=%zu gap=%d(%s) show=%s",
        lw, rw, gap_width, gap_method, draw_sides
    );
}
