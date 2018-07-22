#include "editor.h"
#include "screen.h"
#include "term-info.h"
#include "term-write.h"
#include "util/uchar.h"
#include "view.h"

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

static void calculate_tabbar(Window *win)
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

static void print_horizontal_tab_title(View *v, int idx)
{
    int skip = v->tt_width - v->tt_truncated_width;
    const char *filename = buffer_filename(v->buffer);
    char buf[16];

    if (skip > 0) {
        filename += u_skip_chars(filename, &skip);
    }

    snprintf (
        buf,
        sizeof(buf),
        "%c%d%s",
        obuf.x == 0 && idx > 0 ? '<' : ' ',
        idx + 1,
        buffer_modified(v->buffer) ? "+" : ":"
    );

    if (v == v->window->view) {
        set_builtin_color(BC_ACTIVETAB);
    } else {
        set_builtin_color(BC_INACTIVETAB);
    }

    buf_add_str(buf);
    buf_add_str(filename);
    if (obuf.x == obuf.width - 1 && idx < v->window->views.count - 1) {
        buf_put_char('>');
    } else {
        buf_put_char(' ');
    }
}

static void print_horizontal_tabbar(Window *win)
{
    buf_reset(win->x, win->w, 0);
    terminal.move_cursor(win->x, win->y);

    calculate_tabbar(win);
    int i;
    for (i = win->first_tab_idx; i < win->views.count; i++) {
        View *v = win->views.ptrs[i];
        if (obuf.x + v->tt_truncated_width > win->w) {
            break;
        }
        print_horizontal_tab_title(v, i);
    }
    set_builtin_color(BC_TABBAR);
    if (i != win->views.count) {
        while (obuf.x < obuf.width - 1) {
            buf_put_char(' ');
        }
        if (obuf.x == obuf.width - 1) {
            buf_put_char('>');
        }
    } else {
        buf_clear_eol();
    }
}

static void print_vertical_tab_title(View *v, int idx, int width)
{
    const char *orig_filename = buffer_filename(v->buffer);
    const char *filename = orig_filename;
    int max = editor.options.tab_bar_max_components;
    char buf[16];
    int skip;

    snprintf (
        buf,
        sizeof(buf),
        "%2d%s",
        idx + 1,
        buffer_modified(v->buffer) ? "+" : " "
    );
    if (max) {
        int i, count = 1;

        for (i = 0; filename[i]; i++) {
            if (filename[i] == '/') {
                count++;
            }
        }
        // Ignore first slash because it does not separate components
        if (filename[0] == '/') {
            count--;
        }

        if (count > max) {
            // Skip possible first slash
            for (i = 1; ; i++) {
                if (filename[i] == '/' && --count == max) {
                    i++;
                    break;
                }
            }
            filename += i;
        }
    } else {
        skip = strlen(buf) + u_str_width(filename) - width + 1;
        if (skip > 0) {
            filename += u_skip_chars(filename, &skip);
        }
    }
    if (filename != orig_filename) {
        // filename was shortened. Add "<<" symbol.
        size_t i = strlen(buf);
        u_set_char(buf, &i, 0xab);
        buf[i] = '\0';
    }

    if (v == v->window->view) {
        set_builtin_color(BC_ACTIVETAB);
    } else {
        set_builtin_color(BC_INACTIVETAB);
    }
    buf_add_str(buf);
    buf_add_str(filename);
    buf_clear_eol();
}

static void print_vertical_tabbar(Window *win)
{
    int width = vertical_tabbar_width(win);
    int h = win->edit_h;
    int i, n, cur_idx = 0;

    for (i = 0; i < win->views.count; i++) {
        if (win->view == win->views.ptrs[i]) {
            cur_idx = i;
            break;
        }
    }
    if (win->views.count <= h) {
        // All tabs fit
        win->first_tab_idx = 0;
    } else {
        int max_y = win->first_tab_idx + h - 1;

        if (win->first_tab_idx > cur_idx) {
            win->first_tab_idx = cur_idx;
        }
        if (cur_idx > max_y) {
            win->first_tab_idx += cur_idx - max_y;
        }
    }

    buf_reset(win->x, width, 0);
    n = h;
    if (n + win->first_tab_idx > win->views.count) {
        n = win->views.count - win->first_tab_idx;
    }
    for (i = 0; i < n; i++) {
        int idx = win->first_tab_idx + i;
        obuf.x = 0;
        terminal.move_cursor(win->x, win->y + i);
        print_vertical_tab_title(win->views.ptrs[idx], idx, width);
    }
    set_builtin_color(BC_TABBAR);
    for (; i < h; i++) {
        obuf.x = 0;
        terminal.move_cursor(win->x, win->y + i);
        buf_clear_eol();
    }
}

void print_tabbar(Window *win)
{
    switch (tabbar_visibility(win)) {
    case TAB_BAR_HORIZONTAL:
        print_horizontal_tabbar(win);
        break;
    case TAB_BAR_VERTICAL:
        print_vertical_tabbar(win);
        break;
    default:
        break;
    }
}
