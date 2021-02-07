#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "editor.h"
#include "alias.h"
#include "buffer.h"
#include "error.h"
#include "mode.h"
#include "regexp.h"
#include "screen.h"
#include "terminal/input.h"
#include "terminal/mode.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/exitcode.h"
#include "util/hashset.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "view.h"
#include "window.h"
#include "../build/version.h"

EditorState editor = {
    .status = EDITOR_INITIALIZING,
    .input_mode = INPUT_NORMAL,
    .child_controls_terminal = false,
    .everything_changed = false,
    .resized = false,
    .exit_code = EX_OK,
    .buffers = PTR_ARRAY_INIT,
    .version = version,
    .cmdline_x = 0,
    .cmdline = {
        .buf = STRING_INIT,
        .pos = 0,
        .search_pos = NULL,
        .search_text = NULL
    },
    .command_history = {
        .filename = NULL,
        .max_entries = 512,
        .entries = HASHMAP_INIT
    },
    .search_history = {
        .filename = NULL,
        .max_entries = 128,
        .entries = HASHMAP_INIT
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
        .crlf_newlines = false,
        .display_invisible = false,
        .display_special = false,
        .esc_timeout = 100,
        .filesize_limit = 250,
        .lock_files = true,
        .scroll_margin = 0,
        .select_cursor_char = true,
        .set_window_title = false,
        .show_line_numbers = false,
        .statusline_left = " %f%s%m%r%s%M",
        .statusline_right = " %y,%X   %u   %E%s%b%s%n %t   %p ",
        .tab_bar = true,
        .utf8_bom = false,
    }
};

void init_editor_state(void)
{
    const char *home = getenv("HOME");
    const char *dte_home = getenv("DTE_HOME");
    editor.xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");

    editor.home_dir = strview_intern(home ? home : "");

    if (dte_home) {
        editor.user_config_dir = xstrdup(dte_home);
    } else {
        editor.user_config_dir = xasprintf("%s/.dte", editor.home_dir.data);
    }

    log_init("DTE_LOG");
    DEBUG_LOG("version: %s", version);

    const char *locale = setlocale(LC_CTYPE, "");
    if (unlikely(!locale)) {
        fatal_error("setlocale", errno);
    }

    DEBUG_LOG("locale: %s", locale);
    editor.charset = encoding_from_name(nl_langinfo(CODESET));
    editor.term_utf8 = (editor.charset.type == UTF8);

    // Allow child processes to detect that they're running under dte
    if (unlikely(setenv("DTE_VERSION", version, true) != 0)) {
        fatal_error("setenv", errno);
    }

    regexp_init_word_boundary_tokens();
    init_aliases();
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
    switch (editor.input_mode) {
    case INPUT_NORMAL:
        term_move_cursor (
            window->edit_x + view->cx_display - view->vx,
            window->edit_y + view->cy - view->vy
        );
        break;
    case INPUT_COMMAND:
    case INPUT_SEARCH:
        term_move_cursor(editor.cmdline_x, terminal.height - 1);
        break;
    default:
        BUG("unhandled input mode");
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

    buffer->changed_line_min = LONG_MAX;
    buffer->changed_line_max = -1;
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
        update_line_numbers(w, v->buffer->changed_line_max == LONG_MAX);
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
static void update_buffer_windows(const Buffer *b)
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
    update_term_title(buffer);
    update_all_windows();
    update_command_line();
    end_update();
}

static void ui_resize(void)
{
    if (editor.status == EDITOR_INITIALIZING) {
        return;
    }
    editor.resized = false;
    update_screen_size();
    normal_update();
}

void ui_start(void)
{
    if (editor.status == EDITOR_INITIALIZING) {
        return;
    }
    terminal.put_control_code(terminal.control_codes.init);
    terminal.put_control_code(terminal.control_codes.keypad_on);
    terminal.put_control_code(terminal.control_codes.cup_mode_on);
    ui_resize();
}

void ui_end(void)
{
    if (editor.status == EDITOR_INITIALIZING) {
        return;
    }
    terminal.clear_screen();
    term_move_cursor(0, terminal.height - 1);
    term_show_cursor();
    terminal.put_control_code(terminal.control_codes.cup_mode_off);
    terminal.put_control_code(terminal.control_codes.keypad_off);
    terminal.put_control_code(terminal.control_codes.deinit);
    term_output_flush();
    term_cooked();
}

void suspend(void)
{
    if (getpid() == getsid(0)) {
        error_msg("Session leader can't suspend");
        return;
    }
    if (
        !editor.child_controls_terminal
        && editor.status != EDITOR_INITIALIZING
    ) {
        ui_end();
    }
    int r = kill(0, SIGSTOP);
    if (unlikely(r != 0)) {
        perror_msg("kill");
        term_raw();
        ui_start();
    }
}

const char *editor_file(const char *name)
{
    char buf[4096];
    size_t n = xsnprintf(buf, sizeof buf, "%s/%s", editor.user_config_dir, name);
    return mem_intern(buf, n);
}

static char get_choice(const char *choices)
{
    KeyCode key;
    if (!term_read_key(&key)) {
        return 0;
    }

    switch (key) {
    case KEY_PASTE:
        term_discard_paste();
        return 0;
    case CTRL('C'):
    case CTRL('G'):
    case CTRL('['):
        return 0x18; // Cancel
    case KEY_ENTER:
        return choices[0]; // Default
    }

    if (key < 128) {
        char ch = ascii_tolower(key);
        if (strchr(choices, ch)) {
            return ch;
        }
    }
    return 0;
}

static void show_dialog(const char *question)
{
    unsigned int question_width = u_str_width(question);
    unsigned int min_width = question_width + 2;
    if (terminal.height < 12 || terminal.width < min_width) {
        return;
    }

    unsigned int height = terminal.height / 4;
    unsigned int mid = terminal.height / 2;
    unsigned int top = mid - (height / 2);
    unsigned int bot = top + height;
    unsigned int width = MAX(terminal.width / 2, min_width);
    unsigned int x = (terminal.width - width) / 2;

    // The "underline" and "strikethrough" attributes should only apply
    // to the text, not the whole dialog background:
    const TermColor *text_color = &builtin_colors[BC_DIALOG];
    TermColor dialog_color = *text_color;
    dialog_color.attr &= ~(ATTR_UNDERLINE | ATTR_STRIKETHROUGH);
    set_color(&dialog_color);

    for (unsigned int y = top; y < bot; y++) {
        term_output_reset(x, width, 0);
        term_move_cursor(x, y);
        if (y == mid) {
            term_set_bytes(' ', (width - question_width) / 2);
            set_color(text_color);
            term_add_str(question);
            set_color(&dialog_color);
        }
        term_clear_eol();
    }
}

char dialog_prompt(const char *question, const char *choices)
{
    normal_update();
    term_hide_cursor();
    show_dialog(question);
    show_message(question, false);
    term_output_flush();

    char choice;
    while ((choice = get_choice(choices)) == 0) {
        if (!editor.resized) {
            continue;
        }
        ui_resize();
        term_hide_cursor();
        show_dialog(question);
        show_message(question, false);
        term_output_flush();
    }

    mark_everything_changed();
    return (choice >= 'a') ? choice : 0;
}

char status_prompt(const char *question, const char *choices)
{
    // update_windows() assumes these have been called for the current view
    view_update_cursor_x(view);
    view_update_cursor_y(view);
    view_update(view);

    // Set changed_line_min and changed_line_max before calling update_range()
    mark_all_lines_changed(buffer);

    start_update();
    update_term_title(buffer);
    update_buffer_windows(buffer);
    show_message(question, false);
    end_update();

    char choice;
    while ((choice = get_choice(choices)) == 0) {
        if (!editor.resized) {
            continue;
        }
        ui_resize();
        term_hide_cursor();
        show_message(question, false);
        restore_cursor();
        term_show_cursor();
        term_output_flush();
    }

    return (choice >= 'a') ? choice : 0;
}

typedef struct {
    bool is_modified;
    unsigned long id;
    long cy;
    long vx;
    long vy;
} ScreenState;

static void update_screen(const ScreenState *s)
{
    if (editor.everything_changed) {
        normal_update();
        editor.everything_changed = false;
        return;
    }

    view_update_cursor_x(view);
    view_update_cursor_y(view);
    view_update(view);

    if (s->id == buffer->id) {
        if (s->vx != view->vx || s->vy != view->vy) {
            mark_all_lines_changed(buffer);
        } else {
            // Because of trailing whitespace highlighting and
            // highlighting current line in different color
            // the lines cy (old cursor y) and v->cy need
            // to be updated.
            //
            // Always update at least current line.
            buffer_mark_lines_changed(buffer, s->cy, view->cy);
        }
        if (s->is_modified != buffer_modified(buffer)) {
            mark_buffer_tabbars_changed(buffer);
        }
    } else {
        window->update_tabbar = true;
        mark_all_lines_changed(buffer);
    }

    start_update();
    if (window->update_tabbar) {
        update_term_title(buffer);
    }
    update_buffer_windows(buffer);
    update_command_line();
    end_update();
}

void main_loop(void)
{
    while (editor.status == EDITOR_RUNNING) {
        if (editor.resized) {
            ui_resize();
        }

        KeyCode key;
        if (!term_read_key(&key)) {
            continue;
        }

        clear_error();
        const ScreenState s = {
            .is_modified = buffer_modified(buffer),
            .id = buffer->id,
            .cy = view->cy,
            .vx = view->vx,
            .vy = view->vy
        };

        handle_input(key);
        sanity_check();
        update_screen(&s);
    }
}
