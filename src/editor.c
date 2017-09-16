#include <locale.h>
#include <langinfo.h>
#include "editor.h"
#include "buffer.h"
#include "window.h"
#include "view.h"
#include "term.h"
#include "obuf.h"
#include "search.h"
#include "screen.h"
#include "config.h"
#include "command.h"
#include "error.h"

extern const EditorModeOps normal_mode_ops;
extern const EditorModeOps command_mode_ops;
extern const EditorModeOps search_mode_ops;
extern const EditorModeOps git_open_ops;

EditorState editor = {
    .cmdline = CMDLINE_INIT,
    .version = VERSION,
    .pkgdatadir = PKGDATADIR,
    .mode_ops = {
        [INPUT_NORMAL] = &normal_mode_ops,
        [INPUT_COMMAND] = &command_mode_ops,
        [INPUT_SEARCH] = &search_mode_ops,
        [INPUT_GIT_OPEN] = &git_open_ops
    }
};

void init_editor_state(void)
{
    const char *const home = getenv("HOME");
    const char *const dte_home = getenv("DTE_HOME");

    editor.home_dir = xstrdup(home ? home : "");

    if (dte_home) {
        editor.user_config_dir = xstrdup(dte_home);
    } else {
        editor.user_config_dir = xsprintf("%s/.dte", editor.home_dir);
    }

    setlocale(LC_CTYPE, "");
    editor.charset = nl_langinfo(CODESET);
    if (streq(editor.charset, "UTF-8")) {
        term_utf8 = true;
    }
}

static void sanity_check(void)
{
    View *v = window->view;
    Block *blk;

    if (!DEBUG) {
        return;
    }

    list_for_each_entry(blk, &v->buffer->blocks, node) {
        if (blk == v->cursor.blk) {
            BUG_ON(v->cursor.offset > v->cursor.blk->size);
            return;
        }
    }
    BUG("cursor not seen\n");
}

void set_input_mode(InputMode mode)
{
    editor.input_mode = mode;
}

void any_key(void)
{
    int key;

    puts("Press any key to continue");
    while (!term_read_key(&key)) {
        ;
    }
    if (key == KEY_PASTE) {
        term_discard_paste();
    }
}

static void show_message(const char *msg, bool is_error)
{
    buf_reset(0, screen_w, 0);
    buf_move_cursor(0, screen_h - 1);
    print_message(msg, is_error);
    buf_clear_eol();
}

static void update_command_line(void)
{
    char prefix = ':';

    buf_reset(0, screen_w, 0);
    buf_move_cursor(0, screen_h - 1);
    switch (editor.input_mode) {
    case INPUT_NORMAL:
        print_message(error_buf, msg_is_error);
        break;
    case INPUT_SEARCH:
        prefix = current_search_direction() == SEARCH_FWD ? '/' : '?';
        // fallthrough
    case INPUT_COMMAND:
        editor.cmdline_x = print_command(prefix);
        break;
    case INPUT_GIT_OPEN:
        break;
    }
    buf_clear_eol();
}

static void update_window_full(Window *w)
{
    View *v = w->view;

    view_update_cursor_x(v);
    view_update_cursor_y(v);
    view_update(v);
    print_tabbar(w);
    if (options.show_line_numbers) {
        update_line_numbers(w, true);
    }
    update_range(v, v->vy, v->vy + w->edit_h);
    update_status_line(w);
}

static void restore_cursor(void)
{
    View *v = window->view;
    switch (editor.input_mode) {
    case INPUT_NORMAL:
        buf_move_cursor(
            window->edit_x + v->cx_display - v->vx,
            window->edit_y + v->cy - v->vy);
        break;
    case INPUT_COMMAND:
    case INPUT_SEARCH:
        buf_move_cursor(editor.cmdline_x, screen_h - 1);
        break;
    case INPUT_GIT_OPEN:
        break;
    }
}

static void start_update(void)
{
    buf_hide_cursor();
}

static void clear_update_tabbar(Window *w)
{
    w->update_tabbar = false;
}

static void end_update(void)
{
    restore_cursor();
    buf_show_cursor();
    buf_flush();

    window->view->buffer->changed_line_min = INT_MAX;
    window->view->buffer->changed_line_max = -1;
    for_each_window(clear_update_tabbar);
}

static void update_all_windows(void)
{
    update_window_sizes();
    for_each_window(update_window_full);
    update_separators();
}

static void update_window(Window *w)
{
    View *v = w->view;
    int y1, y2;

    if (w->update_tabbar) {
        print_tabbar(w);
    }

    if (options.show_line_numbers) {
        // Force updating lines numbers if all lines changed
        update_line_numbers(w, v->buffer->changed_line_max == INT_MAX);
    }

    y1 = v->buffer->changed_line_min;
    y2 = v->buffer->changed_line_max;
    if (y1 < v->vy) {
        y1 = v->vy;
    }
    if (y2 > v->vy + w->edit_h - 1) {
        y2 = v->vy + w->edit_h - 1;
    }

    update_range(v, y1, y2 + 1);
    update_status_line(w);
}

// Update all visible views containing this buffer
static void update_buffer_windows(Buffer *b)
{
    long i;

    for (i = 0; i < b->views.count; i++) {
        View *v = b->views.ptrs[i];
        if (v->window->view == v) {
            // Visible view
            if (v != window->view) {
                // Restore cursor
                v->cursor.blk = BLOCK(v->buffer->blocks.next);
                block_iter_goto_offset(&v->cursor, v->saved_cursor_offset);

                // These have already been updated for current view
                view_update_cursor_x(v);
                view_update_cursor_y(v);
                view_update(v);
            }
            update_window(v->window);
        }
    }
}

void normal_update(void)
{
    start_update();
    update_term_title(window->view->buffer);
    update_all_windows();
    update_command_line();
    end_update();
}

void resize(void)
{
    editor.resized = false;
    update_screen_size();

    // "dtach -r winch" sends SIGWINCH after program has been attached
    if (term_cap.strings[STR_CAP_CMD_ks]) {
        // Turn keypad on (makes cursor keys work)
        buf_escape(term_cap.strings[STR_CAP_CMD_ks]);
    }
    if (term_cap.strings[STR_CAP_CMD_ti]) {
        // Use alternate buffer if possible
        buf_escape(term_cap.strings[STR_CAP_CMD_ti]);
    }

    editor.mode_ops[editor.input_mode]->update();
}

void ui_end(void)
{
    struct term_color color = {-1, -1, 0};

    buf_set_color(&color);
    buf_move_cursor(0, screen_h - 1);
    buf_show_cursor();

    // Back to main buffer
    if (term_cap.strings[STR_CAP_CMD_te]) {
        buf_escape(term_cap.strings[STR_CAP_CMD_te]);
    }

    // Turn keypad off
    if (term_cap.strings[STR_CAP_CMD_ke]) {
        buf_escape(term_cap.strings[STR_CAP_CMD_ke]);
    }

    buf_flush();
    term_cooked();
}

void suspend(void)
{
    if (getpid() == getsid(0)) {
        // Session leader can't suspend
        return;
    }
    if (
        !editor.child_controls_terminal
        && editor.status != EDITOR_INITIALIZING
    ) {
        ui_end();
    }
    kill(0, SIGSTOP);
}

char *editor_file(const char *name)
{
    return xsprintf("%s/%s", editor.user_config_dir, name);
}

char get_confirmation(const char *choices, const char *format, ...)
{
    View *v = window->view;
    char buf[4096];
    unsigned int key;
    int pos, i, count = strlen(choices);
    char def = 0;
    va_list ap;

    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    pos = strlen(buf);
    buf[pos++] = ' ';
    buf[pos++] = '[';
    for (i = 0; i < count; i++) {
        if (isupper(choices[i])) {
            def = tolower(choices[i]);
        }
        buf[pos++] = choices[i];
        buf[pos++] = '/';
    }
    pos--;
    buf[pos++] = ']';
    buf[pos] = 0;

    // update_windows() assumes these have been called for the current view
    view_update_cursor_x(v);
    view_update_cursor_y(v);
    view_update(v);

    // Set changed_line_min and changed_line_max before calling update_range()
    mark_all_lines_changed(v->buffer);

    start_update();
    update_term_title(v->buffer);
    update_buffer_windows(v->buffer);
    show_message(buf, false);
    end_update();

    while (1) {
        if (term_read_key(&key)) {
            if (key == KEY_PASTE) {
                term_discard_paste();
                continue;
            }
            if (key == CTRL('C')) {
                key = 0;
                break;
            }
            if (key == KEY_ENTER && def) {
                key = def;
                break;
            }
            if (key > 127) {
                continue;
            }
            key = tolower(key);
            if (strchr(choices, key)) {
                break;
            }
            if (key == def) {
                break;
            }
        } else if (editor.resized) {
            resize();
        }
    }
    return key;
}

typedef struct {
    bool is_modified;
    int id;
    int cy;
    int vx;
    int vy;
} ScreenState;

static void save_state(ScreenState *s, View *v)
{
    s->is_modified = buffer_modified(v->buffer);
    s->id = v->buffer->id;
    s->cy = v->cy;
    s->vx = v->vx;
    s->vy = v->vy;
}

static void update_screen(ScreenState *s)
{
    View *v = window->view;
    Buffer *b = v->buffer;

    if (everything_changed) {
        editor.mode_ops[editor.input_mode]->update();
        everything_changed = false;
        return;
    }

    view_update_cursor_x(v);
    view_update_cursor_y(v);
    view_update(v);

    if (s->id == b->id) {
        if (s->vx != v->vx || s->vy != v->vy) {
            mark_all_lines_changed(b);
        } else {
            // Because of trailing whitespace highlighting and
            // highlighting current line in different color
            // the lines cy (old cursor y) and v->cy need
            // to be updated.
            //
            // Always update at least current line.
            buffer_mark_lines_changed(b, s->cy, v->cy);
        }
        if (s->is_modified != buffer_modified(b)) {
            mark_buffer_tabbars_changed(b);
        }
    } else {
        window->update_tabbar = true;
        mark_all_lines_changed(b);
    }

    start_update();
    if (window->update_tabbar) {
        update_term_title(b);
    }
    update_buffer_windows(b);
    update_command_line();
    end_update();
}

void set_signal_handler(int signum, void (*handler)(int))
{
    struct sigaction act;

    memzero(&act);
    sigemptyset(&act.sa_mask);
    act.sa_handler = handler;
    sigaction(signum, &act, NULL);
}

void main_loop(void)
{
    while (editor.status == EDITOR_RUNNING) {
        int key;

        if (editor.resized) {
            resize();
        }
        if (!term_read_key(&key)) {
            continue;
        }

        clear_error();
        if (editor.input_mode == INPUT_GIT_OPEN) {
            editor.mode_ops[editor.input_mode]->keypress(key);
            editor.mode_ops[editor.input_mode]->update();
        } else {
            ScreenState s;
            save_state(&s, window->view);
            editor.mode_ops[editor.input_mode]->keypress(key);
            sanity_check();
            if (editor.input_mode == INPUT_GIT_OPEN) {
                editor.mode_ops[editor.input_mode]->update();
            } else {
                update_screen(&s);
            }
        }
    }
}
