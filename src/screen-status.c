#include "screen.h"
#include "status.h"
#include "util/utf8.h"

void update_status_line (
    Terminal *term,
    const ColorScheme *colors,
    const GlobalOptions *opts,
    const Window *win,
    InputMode mode
) {
    char lbuf[256], rbuf[256];
    sf_format(win, opts, mode, lbuf, sizeof lbuf, opts->statusline_left);
    sf_format(win, opts, mode, rbuf, sizeof rbuf, opts->statusline_right);

    TermOutputBuffer *obuf = &term->obuf;
    size_t lw = u_str_width(lbuf);
    size_t rw = u_str_width(rbuf);
    term_output_reset(term, win->x, win->w, 0);
    term_move_cursor(obuf, win->x, win->y + win->h - 1);
    set_builtin_color(term, colors, BC_STATUSLINE);

    if (lw + rw <= win->w) {
        // Both fit
        term_add_str(obuf, lbuf);
        term_set_bytes(term, ' ', win->w - lw - rw);
        term_add_str(obuf, rbuf);
    } else if (lw <= win->w && rw <= win->w) {
        // Both would fit separately, draw overlapping
        term_add_str(obuf, lbuf);
        obuf->x = win->w - rw;
        term_move_cursor(obuf, win->x + win->w - rw, win->y + win->h - 1);
        term_add_str(obuf, rbuf);
    } else if (lw <= win->w) {
        // Left fits
        term_add_str(obuf, lbuf);
        term_clear_eol(term);
    } else if (rw <= win->w) {
        // Right fits
        term_set_bytes(term, ' ', win->w - rw);
        term_add_str(obuf, rbuf);
    } else {
        term_clear_eol(term);
    }
}
