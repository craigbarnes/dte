#include "ui.h"
#include "status.h"

void update_status_line(const Window *window)
{
    EditorState *e = window->editor;
    const GlobalOptions *opts = &e->options;
    InputMode mode = e->input_mode;
    char lbuf[512], rbuf[512];
    sf_format(window, opts, mode, lbuf, sizeof lbuf, opts->statusline_left);
    sf_format(window, opts, mode, rbuf, sizeof rbuf, opts->statusline_right);

    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;
    size_t lw = u_str_width(lbuf);
    size_t rw = u_str_width(rbuf);
    unsigned int x = window->x;
    unsigned int y = (window->y + window->h) - 1;
    unsigned int w = window->w;
    term_output_reset(term, x, w, 0);
    term_move_cursor(obuf, x, y);
    set_builtin_style(term, &e->styles, BSE_STATUSLINE);

    if (lw <= w && rw <= w) {
        term_put_str(obuf, lbuf);
        if (lw + rw <= w) {
            // Both fit; clear the inner space
            term_clear_eol(term);
        } else {
            // Both would fit separately; draw overlapping
        }
        obuf->x = w - rw;
        term_move_cursor(obuf, x + w - rw, y);
        term_put_str(obuf, rbuf);
    } else if (lw <= w) {
        // Left fits
        term_put_str(obuf, lbuf);
        term_clear_eol(term);
    } else if (rw <= w) {
        // Right fits
        term_set_bytes(term, ' ', w - rw);
        term_put_str(obuf, rbuf);
    } else {
        term_clear_eol(term);
    }
}
