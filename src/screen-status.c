#include "screen.h"
#include "status.h"

void update_status_line(const Window *window)
{
    EditorState *e = window->editor;
    const GlobalOptions *opts = &e->options;
    InputMode mode = e->input_mode;
    char lbuf[512], rbuf[512];
    sf_format(window, opts, mode, lbuf, sizeof lbuf, opts->statusline_left);
    sf_format(window, opts, mode, rbuf, sizeof rbuf, opts->statusline_right);

    const ColorScheme *colors = &e->colors;
    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;
    size_t lw = u_str_width(lbuf);
    size_t rw = u_str_width(rbuf);
    int w = window->w;
    static_assert_compatible_types(w, window->w);
    term_output_reset(term, window->x, w, 0);
    term_move_cursor(obuf, window->x, window->y + window->h - 1);
    set_builtin_color(term, colors, BC_STATUSLINE);

    if (lw + rw <= w) {
        // Both fit
        term_add_str(obuf, lbuf);
        term_set_bytes(term, ' ', w - lw - rw);
        term_add_str(obuf, rbuf);
    } else if (lw <= w && rw <= w) {
        // Both would fit separately, draw overlapping
        term_add_str(obuf, lbuf);
        obuf->x = w - rw;
        term_move_cursor(obuf, window->x + w - rw, window->y + window->h - 1);
        term_add_str(obuf, rbuf);
    } else if (lw <= w) {
        // Left fits
        term_add_str(obuf, lbuf);
        term_clear_eol(term);
    } else if (rw <= w) {
        // Right fits
        term_set_bytes(term, ' ', w - rw);
        term_add_str(obuf, rbuf);
    } else {
        term_clear_eol(term);
    }
}
