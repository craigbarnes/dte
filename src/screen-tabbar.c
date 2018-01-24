#include "screen.h"
#include "tabbar.h"
#include "editor.h"
#include "uchar.h"
#include "obuf.h"
#include "view.h"

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
    buf_move_cursor(win->x, win->y);

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
        buf[i] = 0;
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
        buf_move_cursor(win->x, win->y + i);
        print_vertical_tab_title(win->views.ptrs[idx], idx, width);
    }
    set_builtin_color(BC_TABBAR);
    for (; i < h; i++) {
        obuf.x = 0;
        buf_move_cursor(win->x, win->y + i);
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
