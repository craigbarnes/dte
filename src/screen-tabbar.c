#include "screen.h"
#include "editor.h"
#include "terminal/output.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/strtonum.h"
#include "util/utf8.h"

static size_t tab_title_width(size_t tab_number, const char *filename)
{
    return 3 + size_str_width(tab_number) + u_str_width(filename);
}

static void update_tab_title_width(View *v, size_t tab_number)
{
    size_t w = tab_title_width(tab_number, buffer_filename(v->buffer));
    v->tt_width = w;
    v->tt_truncated_width = w;
}

static void update_first_tab_idx(Window *win)
{
    size_t max_first_idx = win->views.count;
    for (size_t w = 0; max_first_idx > 0; max_first_idx--) {
        const View *v = win->views.ptrs[max_first_idx - 1];
        w += v->tt_truncated_width;
        if (w > win->w) {
            break;
        }
    }

    size_t min_first_idx = win->views.count;
    for (size_t w = 0; min_first_idx > 0; min_first_idx--) {
        const View *v = win->views.ptrs[min_first_idx - 1];
        if (w || v == win->view) {
            w += v->tt_truncated_width;
        }
        if (w > win->w) {
            break;
        }
    }

    if (win->first_tab_idx < min_first_idx) {
        win->first_tab_idx = min_first_idx;
    }
    if (win->first_tab_idx > max_first_idx) {
        win->first_tab_idx = max_first_idx;
    }
}

static void calculate_tabbar(Window *win)
{
    int total_w = 0;
    for (size_t i = 0, n = win->views.count; i < n; i++) {
        View *v = win->views.ptrs[i];
        if (v == win->view) {
            // Make sure current tab is visible
            if (win->first_tab_idx > i) {
                win->first_tab_idx = i;
            }
        }
        update_tab_title_width(v, i + 1);
        total_w += v->tt_width;
    }

    if (total_w <= win->w) {
        // All tabs fit without truncating
        win->first_tab_idx = 0;
        return;
    }

    // Truncate all wide tabs
    total_w = 0;
    int truncated_count = 0;
    for (size_t i = 0, n = win->views.count; i < n; i++) {
        View *v = win->views.ptrs[i];
        int truncated_w = 20;
        if (v->tt_width > truncated_w) {
            v->tt_truncated_width = truncated_w;
            total_w += truncated_w;
            truncated_count++;
        } else {
            total_w += v->tt_width;
        }
    }

    if (total_w > win->w) {
        // Not all tabs fit even after truncating wide tabs
        update_first_tab_idx(win);
        return;
    }

    // All tabs fit after truncating wide tabs
    int extra = win->w - total_w;

    // Divide extra space between truncated tabs
    while (extra > 0) {
        BUG_ON(truncated_count == 0);
        int extra_avg = extra / truncated_count;
        int extra_mod = extra % truncated_count;

        for (size_t i = 0, n = win->views.count; i < n; i++) {
            View *v = win->views.ptrs[i];
            int add = v->tt_width - v->tt_truncated_width;
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

            v->tt_truncated_width += add;
            extra -= add;
        }
    }

    win->first_tab_idx = 0;
}

static void print_tab_title(const View *v, size_t idx)
{
    const char *filename = buffer_filename(v->buffer);
    int skip = v->tt_width - v->tt_truncated_width;
    if (skip > 0) {
        filename += u_skip_chars(filename, &skip);
    }

    const char *tab_number = uint_to_str((unsigned int)idx + 1);
    bool is_active_tab = (v == v->window->view);
    bool is_modified = buffer_modified(v->buffer);
    bool left_overflow = (obuf.x == 0 && idx > 0);

    set_builtin_color(is_active_tab ? BC_ACTIVETAB : BC_INACTIVETAB);
    term_put_char(left_overflow ? '<' : ' ');
    term_add_str(tab_number);
    term_put_char(is_modified ? '+' : ':');
    term_add_str(filename);

    size_t ntabs = v->window->views.count;
    bool right_overflow = (obuf.x == (obuf.width - 1) && idx < (ntabs - 1));
    term_put_char(right_overflow ? '>' : ' ');
}

void print_tabbar(Window *win)
{
    if (!editor.options.tab_bar) {
        return;
    }

    term_output_reset(win->x, win->w, 0);
    term_move_cursor(win->x, win->y);
    calculate_tabbar(win);

    size_t i = win->first_tab_idx;
    size_t n = win->views.count;
    for (; i < n; i++) {
        const View *v = win->views.ptrs[i];
        if (obuf.x + v->tt_truncated_width > win->w) {
            break;
        }
        print_tab_title(v, i);
    }

    set_builtin_color(BC_TABBAR);

    if (i == n) {
        term_clear_eol();
        return;
    }

    while (obuf.x < obuf.width - 1) {
        term_put_char(' ');
    }
    if (obuf.x == obuf.width - 1) {
        term_put_char('>');
    }
}
