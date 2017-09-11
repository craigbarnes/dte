#include "tabbar.h"
#include "view.h"
#include "uchar.h"

static int tab_title_width(int number, const char *filename)
{
    return 3 + number_width(number) + u_str_width(filename);
}

static void update_tab_title_width(View *v, int tab_number)
{
    int w = tab_title_width(tab_number, buffer_filename(v->buffer));

    v->tt_width = w;
    v->tt_truncated_width = w;
}

static void update_first_tab_idx(Window *win)
{
    int min_first_idx, max_first_idx, w;

    w = 0;
    for (max_first_idx = win->views.count; max_first_idx > 0; max_first_idx--) {
        View *v = win->views.ptrs[max_first_idx - 1];
        w += v->tt_truncated_width;
        if (w > win->w) {
            break;
        }
    }

    w = 0;
    for (min_first_idx = win->views.count; min_first_idx > 0; min_first_idx--) {
        View *v = win->views.ptrs[min_first_idx - 1];
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

void calculate_tabbar(Window *win)
{
    int extra, i, truncated_count, total_w = 0;

    for (i = 0; i < win->views.count; i++) {
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
    truncated_count = 0;
    for (i = 0; i < win->views.count; i++) {
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
    extra = win->w - total_w;

    // Divide extra space between truncated tabs
    while (extra > 0) {
        int extra_avg = extra / truncated_count;
        int extra_mod = extra % truncated_count;

        for (i = 0; i < win->views.count; i++) {
            View *v = win->views.ptrs[i];
            int add = v->tt_width - v->tt_truncated_width;
            int avail;

            if (add == 0) {
                continue;
            }

            avail = extra_avg;
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
