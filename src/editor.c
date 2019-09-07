#include <langinfo.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "editor.h"
#include "buffer.h"
#include "command.h"
#include "config.h"
#include "debug.h"
#include "error.h"
#include "mode.h"
#include "screen.h"
#include "search.h"
#include "terminal/input.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "util/ascii.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "view.h"
#include "window.h"
#include "../build/version.h"

static volatile sig_atomic_t terminal_resized;

static void resize(void);
static void ui_end(void);

EditorState editor = {
    .status = EDITOR_INITIALIZING,
    .input_mode = INPUT_NORMAL,
    .child_controls_terminal = false,
    .everything_changed = false,
    .search_history = PTR_ARRAY_INIT,
    .command_history = PTR_ARRAY_INIT,
    .version = version,
    .cmdline_x = 0,
    .resize = resize,
    .ui_end = ui_end,
    .cmdline = {
        .buf = STRING_INIT,
        .pos = 0,
        .search_pos = -1,
        .search_text = NULL
    },
    .options = {
        .auto_indent = true,
        .detect_indent = 0,
        .editorconfig = false,
        .emulate_tab = false,
        .expand_tab = false,
        .file_history = true,
        .indent_width = 8,
        .syntax = true,
        .tab_width = 8,
        .text_width = 72,
        .ws_error = WSE_SPECIAL,

        // Global-only options
        .case_sensitive_search = CSS_TRUE,
        .display_invisible = false,
        .display_special = false,
        .esc_timeout = 100,
        .lock_files = true,
        .newline = NEWLINE_UNIX,
        .scroll_margin = 0,
        .set_window_title = false,
        .show_line_numbers = false,
        .statusline_left = NULL,
        .statusline_right = NULL,
        .tab_bar = TAB_BAR_HORIZONTAL,
        .tab_bar_max_components = 0,
        .tab_bar_width = 25,
        .filesize_limit = 250,
    },
    .mode_ops = {
        [INPUT_NORMAL] = &normal_mode_ops,
        [INPUT_COMMAND] = &command_mode_ops,
        [INPUT_SEARCH] = &search_mode_ops,
        [INPUT_GIT_OPEN] = &git_open_ops
    }
};

void init_editor_state(void)
{
    const char *const pager = getenv("PAGER");
    const char *const home = getenv("HOME");
    const char *const dte_home = getenv("DTE_HOME");

    editor.pager = xstrdup(pager ? pager : "less");
    editor.home_dir = xstrdup(home ? home : "");

    if (dte_home) {
        editor.user_config_dir = xstrdup(dte_home);
    } else {
        editor.user_config_dir = xasprintf("%s/.dte", editor.home_dir);
    }

    setlocale(LC_CTYPE, "");
    editor.charset = encoding_from_name(nl_langinfo(CODESET));
    editor.term_utf8 = (editor.charset.type == UTF8);

    editor.options.statusline_left = " %f%s%m%r%s%M";
    editor.options.statusline_right = " %y,%X   %u   %E %n %t   %p ";
}

static void sanity_check(void)
{
#if DEBUG >= 1
    View *v = window->view;
    Block *blk;
    block_for_each(blk, &v->buffer->blocks) {
        if (blk == v->cursor.blk) {
            BUG_ON(v->cursor.offset > v->cursor.blk->size);
            return;
        }
    }
    BUG("cursor not seen");
#endif
}

void any_key(void)
{
    fputs("Press any key to continue\r\n", stderr);
    KeyCode key;
    while (!term_read_key(&key)) {
        ;
    }
    if (key == KEY_PASTE) {
        term_discard_paste();
    }
}

static void update_window_full(Window *w)
{
    View *v = w->view;
    view_update_cursor_x(v);
    view_update_cursor_y(v);
    view_update(v);
    print_tabbar(w);
    if (editor.options.show_line_numbers) {
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
        terminal.move_cursor (
            window->edit_x + v->cx_display - v->vx,
            window->edit_y + v->cy - v->vy
        );
        break;
    case INPUT_COMMAND:
    case INPUT_SEARCH:
        terminal.move_cursor(editor.cmdline_x, terminal.height - 1);
        break;
    case INPUT_GIT_OPEN:
        break;
    }
}

static void start_update(void)
{
    term_hide_cursor();
}

static void clear_update_tabbar(Window *w)
{
    w->update_tabbar = false;
}

static void end_update(void)
{
    restore_cursor();
    term_show_cursor();
    term_output_flush();

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
    if (w->update_tabbar) {
        print_tabbar(w);
    }

    View *v = w->view;
    if (editor.options.show_line_numbers) {
        // Force updating lines numbers if all lines changed
        update_line_numbers(w, v->buffer->changed_line_max == INT_MAX);
    }

    long y1 = v->buffer->changed_line_min;
    long y2 = v->buffer->changed_line_max;
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
static void update_buffer_windows(const Buffer *const b)
{
    for (size_t i = 0, n = b->views.count; i < n; i++) {
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

void handle_sigwinch(int UNUSED_ARG(signum))
{
    terminal_resized = true;
}

static void resize(void)
{
    terminal_resized = false;
    update_screen_size();

    // Turn keypad on (makes cursor keys work)
    terminal.put_control_code(terminal.control_codes.keypad_on);

    // Use alternate buffer if possible
    terminal.put_control_code(terminal.control_codes.cup_mode_on);

    editor.mode_ops[editor.input_mode]->update();
}

static void ui_end(void)
{
    terminal.put_control_code(terminal.control_codes.reset_colors);
    terminal.put_control_code(terminal.control_codes.reset_attrs);
    terminal.clear_screen();

    terminal.move_cursor(0, terminal.height - 1);
    term_show_cursor();

    terminal.put_control_code(terminal.control_codes.cup_mode_off);
    terminal.put_control_code(terminal.control_codes.keypad_off);

    term_output_flush();
    terminal.cooked();
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
    return xasprintf("%s/%s", editor.user_config_dir, name);
}

char get_confirmation(const char *choices, const char *format, ...)
{
    char buf[4096];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    size_t pos = strlen(buf);
    buf[pos++] = ' ';
    buf[pos++] = '[';

    unsigned char def = 0;
    for (size_t i = 0, n = strlen(choices); i < n; i++) {
        if (ascii_isupper(choices[i])) {
            def = ascii_tolower(choices[i]);
        }
        buf[pos++] = choices[i];
        buf[pos++] = '/';
    }

    buf[pos - 1] = ']';
    buf[pos] = '\0';

    // update_windows() assumes these have been called for the current view
    View *v = window->view;
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

    KeyCode key;
    while (1) {
        if (term_read_key(&key)) {
            if (key == KEY_PASTE) {
                term_discard_paste();
                continue;
            }
            if (key == CTRL('C') || key == CTRL('G') || key == CTRL('[')) {
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
            key = ascii_tolower(key);
            if (strchr(choices, key)) {
                break;
            }
            if (key == def) {
                break;
            }
        } else if (terminal_resized) {
            resize();
        }
    }
    return key;
}

typedef struct {
    bool is_modified;
    unsigned int id;
    long cy;
    long vx;
    long vy;
} ScreenState;

static void update_screen(const ScreenState *const s)
{
    if (editor.everything_changed) {
        editor.mode_ops[editor.input_mode]->update();
        editor.everything_changed = false;
        return;
    }

    View *v = window->view;
    view_update_cursor_x(v);
    view_update_cursor_y(v);
    view_update(v);

    Buffer *b = v->buffer;
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

void main_loop(void)
{
    while (editor.status == EDITOR_RUNNING) {
        if (terminal_resized) {
            resize();
        }

        KeyCode key;
        if (!term_read_key(&key)) {
            continue;
        }

        clear_error();
        if (editor.input_mode == INPUT_GIT_OPEN) {
            editor.mode_ops[editor.input_mode]->keypress(key);
            editor.mode_ops[editor.input_mode]->update();
        } else {
            const View *v = window->view;
            const ScreenState s = {
                .is_modified = buffer_modified(v->buffer),
                .id = v->buffer->id,
                .cy = v->cy,
                .vx = v->vx,
                .vy = v->vy
            };
            editor.mode_ops[editor.input_mode]->keypress(key);
            sanity_check();
            if (editor.input_mode == INPUT_GIT_OPEN) {
                editor.mode_ops[INPUT_GIT_OPEN]->update();
            } else {
                update_screen(&s);
            }
        }
    }
}
