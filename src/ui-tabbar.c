#include "ui.h"
#include "util/numtostr.h"
#include "util/strtonum.h"

static size_t tab_title_width(size_t tab_number, const char *filename)
{
    return 3 + size_str_width(tab_number) + u_str_width(filename);
}

static size_t get_first_tab_idx(const Window *window)
{
    size_t max_first_idx = window->views.count;
    for (size_t w = 0; max_first_idx > 0; max_first_idx--) {
        const View *view = window->views.ptrs[max_first_idx - 1];
        w += view->tt_truncated_width;
        if (w > window->w) {
            break;
        }
    }

    size_t min_first_idx = window->views.count;
    for (size_t w = 0; min_first_idx > 0; min_first_idx--) {
        const View *view = window->views.ptrs[min_first_idx - 1];
        if (w || view == window->view) {
            w += view->tt_truncated_width;
        }
        if (w > window->w) {
            break;
        }
    }

    return CLAMP(window->first_tab_idx, min_first_idx, max_first_idx);
}

static void calculate_tabbar(Window *window)
{
    const size_t ntabs = window->views.count;
    void **ptrs = window->views.ptrs;
    unsigned int total_w = 0;

    for (size_t i = 0; i < ntabs; i++) {
        View *view = ptrs[i];
        if (view == window->view) {
            // Make sure current tab is visible
            window->first_tab_idx = MIN(i, window->first_tab_idx);
        }
        size_t w = tab_title_width(i + 1, buffer_filename(view->buffer));
        view->tt_width = w;
        view->tt_truncated_width = w;
        total_w += w;
    }

    if (total_w <= window->w) {
        // All tabs fit without truncating
        window->first_tab_idx = 0;
        return;
    }

    // Truncate all wide tabs
    total_w = 0;
    unsigned int truncated_count = 0;
    for (size_t i = 0; i < ntabs; i++) {
        View *view = ptrs[i];
        unsigned int truncated_w = 20;
        if (view->tt_width > truncated_w) {
            view->tt_truncated_width = truncated_w;
            total_w += truncated_w;
            truncated_count++;
        } else {
            total_w += view->tt_width;
        }
    }

    if (total_w > window->w) {
        // Not all tabs fit even after truncating wide tabs
        window->first_tab_idx = get_first_tab_idx(window);
        return;
    }

    // All tabs fit after truncating wide tabs
    unsigned int extra = window->w - total_w;

    // Divide extra space between truncated tabs
    while (extra > 0) {
        BUG_ON(truncated_count == 0);
        unsigned int extra_avg = extra / truncated_count;
        unsigned int extra_mod = extra % truncated_count;

        for (size_t i = 0; i < ntabs; i++) {
            View *view = ptrs[i];
            unsigned int add = view->tt_width - view->tt_truncated_width;
            if (add == 0) {
                continue;
            }

            unsigned int avail = extra_avg;
            if (extra_mod && extra_avg == 0) {
                // This is needed for equal divide
                avail++;
                extra_mod--;
            }

            if (add > avail) {
                add = avail;
            } else {
                truncated_count--;
            }

            BUG_ON(add > extra);
            view->tt_truncated_width += add;
            extra -= add;
        }
    }

    window->first_tab_idx = 0;
}

static void print_tab_title(Terminal *term, const StyleMap *styles, const View *view, size_t idx)
{
    const char *filename = buffer_filename(view->buffer);
    const char *tab_number = uint_to_str((unsigned int)idx + 1);
    TermOutputBuffer *obuf = &term->obuf;
    bool is_active_tab = (view == view->window->view);
    bool is_modified = buffer_modified(view->buffer);
    bool left_overflow = (obuf->x == 0 && idx > 0);

    filename += u_skip_chars(filename, view->tt_width - view->tt_truncated_width);
    set_builtin_style(term, styles, is_active_tab ? BSE_ACTIVETAB : BSE_INACTIVETAB);
    term_put_char(obuf, left_overflow ? '<' : ' ');
    term_put_str(obuf, tab_number);
    term_put_char(obuf, is_modified ? '+' : ':');
    term_put_str(obuf, filename);

    size_t ntabs = view->window->views.count;
    bool right_overflow = (obuf->x == (obuf->width - 1) && idx < (ntabs - 1));
    term_put_char(obuf, right_overflow ? '>' : ' ');
}

void print_tabbar(Terminal *term, const StyleMap *styles, Window *window)
{
    TermOutputBuffer *obuf = &term->obuf;
    term_output_reset(term, window->x, window->w, 0);
    term_move_cursor(obuf, window->x, window->y);
    calculate_tabbar(window);

    size_t i = window->first_tab_idx;
    size_t n = window->views.count;
    for (; i < n; i++) {
        const View *view = window->views.ptrs[i];
        if (obuf->x + view->tt_truncated_width > window->w) {
            break;
        }
        print_tab_title(term, styles, view, i);
    }

    set_builtin_style(term, styles, BSE_TABBAR);

    if (i == n) {
        term_clear_eol(term);
        return;
    }

    while (obuf->x < obuf->width - 1) {
        term_put_char(obuf, ' ');
    }
    if (obuf->x == obuf->width - 1) {
        term_put_char(obuf, '>');
    }
}
