#include "screen.h"
#include "util/numtostr.h"
#include "util/strtonum.h"

static size_t tab_title_width(size_t tab_number, const char *filename)
{
    return 3 + size_str_width(tab_number) + u_str_width(filename);
}

static void update_tab_title_width(View *view, size_t tab_number)
{
    size_t w = tab_title_width(tab_number, buffer_filename(view->buffer));
    view->tt_width = w;
    view->tt_truncated_width = w;
}

static void update_first_tab_idx(Window *window)
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

    size_t idx = CLAMP(window->first_tab_idx, min_first_idx, max_first_idx);
    window->first_tab_idx = idx;
}

static void calculate_tabbar(Window *window)
{
    int total_w = 0;
    for (size_t i = 0, n = window->views.count; i < n; i++) {
        View *view = window->views.ptrs[i];
        if (view == window->view) {
            // Make sure current tab is visible
            window->first_tab_idx = MIN(i, window->first_tab_idx);
        }
        update_tab_title_width(view, i + 1);
        total_w += view->tt_width;
    }

    if (total_w <= window->w) {
        // All tabs fit without truncating
        window->first_tab_idx = 0;
        return;
    }

    // Truncate all wide tabs
    total_w = 0;
    int truncated_count = 0;
    for (size_t i = 0, n = window->views.count; i < n; i++) {
        View *view = window->views.ptrs[i];
        int truncated_w = 20;
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
        update_first_tab_idx(window);
        return;
    }

    // All tabs fit after truncating wide tabs
    int extra = window->w - total_w;

    // Divide extra space between truncated tabs
    while (extra > 0) {
        BUG_ON(truncated_count == 0);
        int extra_avg = extra / truncated_count;
        int extra_mod = extra % truncated_count;

        for (size_t i = 0, n = window->views.count; i < n; i++) {
            View *view = window->views.ptrs[i];
            int add = view->tt_width - view->tt_truncated_width;
            if (add == 0) {
                continue;
            }

            int avail = extra_avg;
            if (extra_mod) {
                // This is needed for equal divide
                if (extra_avg == 0) {
                    avail++;
                    extra_mod--;
                }
            }
            if (add > avail) {
                add = avail;
            } else {
                truncated_count--;
            }

            view->tt_truncated_width += add;
            extra -= add;
        }
    }

    window->first_tab_idx = 0;
}

static void print_tab_title(Terminal *term, const ColorScheme *colors, const View *view, size_t idx)
{
    const char *filename = buffer_filename(view->buffer);
    int skip = view->tt_width - view->tt_truncated_width;
    if (skip > 0) {
        filename += u_skip_chars(filename, &skip);
    }

    const char *tab_number = uint_to_str((unsigned int)idx + 1);
    TermOutputBuffer *obuf = &term->obuf;
    bool is_active_tab = (view == view->window->view);
    bool is_modified = buffer_modified(view->buffer);
    bool left_overflow = (obuf->x == 0 && idx > 0);

    set_builtin_color(term, colors, is_active_tab ? BC_ACTIVETAB : BC_INACTIVETAB);
    term_put_char(obuf, left_overflow ? '<' : ' ');
    term_add_str(obuf, tab_number);
    term_put_char(obuf, is_modified ? '+' : ':');
    term_add_str(obuf, filename);

    size_t ntabs = view->window->views.count;
    bool right_overflow = (obuf->x == (obuf->width - 1) && idx < (ntabs - 1));
    term_put_char(obuf, right_overflow ? '>' : ' ');
}

void print_tabbar(Terminal *term, const ColorScheme *colors, Window *window)
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
        print_tab_title(term, colors, view, i);
    }

    set_builtin_color(term, colors, BC_TABBAR);

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
