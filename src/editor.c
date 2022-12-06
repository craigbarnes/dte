#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "editor.h"
#include "bind.h"
#include "command/macro.h"
#include "commands.h"
#include "compiler.h"
#include "error.h"
#include "file-option.h"
#include "filetype.h"
#include "lock.h"
#include "mode.h"
#include "regexp.h"
#include "screen.h"
#include "search.h"
#include "status.h"
#include "tag.h"
#include "terminal/input.h"
#include "terminal/mode.h"
#include "terminal/xterm.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/exitcode.h"
#include "util/intern.h"
#include "util/log.h"
#include "util/path.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "util/xstdio.h"
#include "window.h"
#include "../build/version.h"

static volatile sig_atomic_t resized = 0;

EditorState editor = {
    .status = EDITOR_INITIALIZING,
    .input_mode = INPUT_NORMAL,
    .child_controls_terminal = false,
    .everything_changed = false,
    .cursor_style_changed = false,
    .compilers = HASHMAP_INIT,
    .syntaxes = HASHMAP_INIT,
    .buffers = PTR_ARRAY_INIT,
    .filetypes = PTR_ARRAY_INIT,
    .file_options = PTR_ARRAY_INIT,
    .bookmarks = PTR_ARRAY_INIT,
    .root_frame = NULL,
    .window = NULL,
    .view = NULL,
    .buffer = NULL,
    .version = VERSION,
    .cmdline_x = 0,
    .colors = {
        .other = HASHMAP_INIT,
    },
    .messages = {
        .array = PTR_ARRAY_INIT,
        .pos = 0,
    },
    .cmdline = {
        .buf = STRING_INIT,
        .pos = 0,
        .search_pos = NULL,
        .search_text = NULL
    },
    .file_history = {
        .filename = NULL,
        .entries = HASHMAP_INIT
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
    .macro = {
        .macro = PTR_ARRAY_INIT,
        .prev_macro = PTR_ARRAY_INIT,
        .insert_buffer = STRING_INIT,
        .recording = false,
    },
    .cursor_styles = {
        [CURSOR_MODE_DEFAULT] = {.type = CURSOR_DEFAULT, .color = COLOR_DEFAULT},
        [CURSOR_MODE_INSERT] = {.type = CURSOR_KEEP, .color = COLOR_KEEP},
        [CURSOR_MODE_OVERWRITE] = {.type = CURSOR_KEEP, .color = COLOR_KEEP},
        [CURSOR_MODE_CMDLINE] = {.type = CURSOR_KEEP, .color = COLOR_KEEP},
    },
    .modes = {
        [INPUT_NORMAL] = {
            .cmds = &normal_commands,
            .aliases = HASHMAP_INIT,
            .key_bindings = INTMAP_INIT,
        },
        [INPUT_COMMAND] = {
            .cmds = &cmd_mode_commands,
            .aliases = HASHMAP_INIT,
            .key_bindings = INTMAP_INIT,
        },
        [INPUT_SEARCH] = {
            .cmds = &search_mode_commands,
            .aliases = HASHMAP_INIT,
            .key_bindings = INTMAP_INIT,
        },
    },
    .terminal = {
        .color_type = TERM_8_COLOR,
        .width = 80,
        .height = 24,
        .parse_input = xterm_parse_key,
    },
    .options = {
        .auto_indent = true,
        .detect_indent = 0,
        .editorconfig = false,
        .emulate_tab = false,
        .expand_tab = false,
        .file_history = true,
        .indent_width = 8,
        .overwrite = false,
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
        .optimize_true_color = true,
        .scroll_margin = 0,
        .select_cursor_char = true,
        .set_window_title = false,
        .show_line_numbers = false,
        .statusline_left = " %f%s%m%s%r%s%M",
        .statusline_right = " %y,%X  %u  %o  %E%s%b%s%n %t   %p ",
        .tab_bar = true,
        .utf8_bom = false,
    }
};

static void set_and_check_locale(void)
{
    const char *default_locale = setlocale(LC_CTYPE, "");
    if (likely(default_locale)) {
        const char *codeset = nl_langinfo(CODESET);
        LOG_INFO("locale: %s (codeset: %s)", default_locale, codeset);
        if (likely(lookup_encoding(codeset) == UTF8)) {
            return;
        }
    } else {
        LOG_ERROR("failed to set default locale");
    }

    static const char fallbacks[][12] = {"C.UTF-8", "en_US.UTF-8"};
    const char *fallback = NULL;
    for (size_t i = 0; i < ARRAYLEN(fallbacks) && !fallback; i++) {
        fallback = setlocale(LC_CTYPE, fallbacks[i]);
    }
    if (fallback) {
        LOG_INFO("using fallback locale for LC_CTYPE: %s", fallback);
        return;
    }

    LOG_ERROR("no UTF-8 fallback locales found");
    fputs("setlocale() failed\n", stderr);
    exit(EX_CONFIG);
}

EditorState *init_editor_state(void)
{
    EditorState *e = &editor;
    BUG_ON(statusline_format_find_error(e->options.statusline_left));
    BUG_ON(statusline_format_find_error(e->options.statusline_right));

    const char *home = getenv("HOME");
    const char *dte_home = getenv("DTE_HOME");
    e->home_dir = strview_intern(home ? home : "");
    if (dte_home) {
        e->user_config_dir = xstrdup(dte_home);
    } else {
        e->user_config_dir = xasprintf("%s/.dte", e->home_dir.data);
    }

    pid_t pid = getpid();
    bool leader = pid == getsid(0);
    e->session_leader = leader;
    LOG_INFO("dte version: " VERSION);
    LOG_INFO("pid: %jd%s", (intmax_t)pid, leader ? " (session leader)" : "");

    set_and_check_locale();
    init_file_locks_context(e->user_config_dir, pid);

    // Allow child processes to detect that they're running under dte
    if (unlikely(setenv("DTE_VERSION", VERSION, true) != 0)) {
        fatal_error("setenv", errno);
    }

    RegexpWordBoundaryTokens *wb = &e->regexp_word_tokens;
    if (regexp_init_word_boundary_tokens(wb)) {
        LOG_INFO("regex word boundary tokens detected: %s %s", wb->start, wb->end);
    } else {
        LOG_WARNING("no regex word boundary tokens detected");
    }

    term_input_init(&e->terminal.ibuf);
    term_output_init(&e->terminal.obuf);
    hashmap_init(&e->modes[INPUT_NORMAL].aliases, 32);
    intmap_init(&e->modes[INPUT_NORMAL].key_bindings, 150);
    intmap_init(&e->modes[INPUT_COMMAND].key_bindings, 40);
    intmap_init(&e->modes[INPUT_SEARCH].key_bindings, 40);
    return e;
}

void free_editor_state(EditorState *e)
{
    free(e->clipboard.buf);
    free_file_options(&e->file_options);
    free_filetypes(&e->filetypes);
    free_syntaxes(&e->syntaxes);
    file_history_free(&e->file_history);
    history_free(&e->command_history);
    history_free(&e->search_history);
    search_free_regexp(&e->search);
    term_output_free(&e->terminal.obuf);
    term_input_free(&e->terminal.ibuf);
    cmdline_free(&e->cmdline);
    clear_messages(&e->messages);
    free_macro(&e->macro);
    tag_file_free(&e->tagfile);

    ptr_array_free_cb(&e->bookmarks, FREE_FUNC(file_location_free));
    ptr_array_free_cb(&e->buffers, FREE_FUNC(free_buffer));
    hashmap_free(&e->compilers, FREE_FUNC(free_compiler));
    hashmap_free(&e->colors.other, free);

    for (size_t i = 0; i < ARRAYLEN(e->modes); i++) {
        ModeHandler *m = &e->modes[i];
        free_bindings(&m->key_bindings);
        hashmap_free(&m->aliases, free);
    }

    free_interned_strings();
    free_interned_regexps();

    // TODO: intern this (so that it's freed by free_intern_pool())
    free((void*)e->user_config_dir);

    // Zero pointers, to help LSan find leaks
    *e = (EditorState){.window = NULL};
}

void handle_sigwinch(int UNUSED_ARG(signum))
{
    resized = 1;
}

static void sanity_check(const View *view)
{
#if DEBUG >= 1
    const Block *blk;
    block_for_each(blk, &view->buffer->blocks) {
        if (blk == view->cursor.blk) {
            BUG_ON(view->cursor.offset > view->cursor.blk->size);
            return;
        }
    }
    BUG("cursor not seen");
#else
    (void)view;
#endif
}

void any_key(Terminal *term, unsigned int esc_timeout)
{
    KeyCode key;
    xfputs("Press any key to continue\r\n", stderr);
    while ((key = term_read_key(term, esc_timeout)) == KEY_NONE) {
        ;
    }
    bool bracketed_paste = key == KEY_BRACKETED_PASTE;
    if (bracketed_paste || key == KEY_DETECTED_PASTE) {
        term_discard_paste(&term->ibuf, bracketed_paste);
    }
}

static void update_window_full(Window *window, void* UNUSED_ARG(data))
{
    EditorState *e = window->editor;
    View *view = window->view;
    view_update_cursor_x(view);
    view_update_cursor_y(view);
    view_update(view, e->options.scroll_margin);
    if (e->options.tab_bar) {
        print_tabbar(&e->terminal, &e->colors, window);
    }
    if (e->options.show_line_numbers) {
        update_line_numbers(&e->terminal, &e->colors, window, true);
    }
    update_range(e, view, view->vy, view->vy + window->edit_h);
    update_status_line(window);
}

static void restore_cursor(EditorState *e)
{
    unsigned int x, y;
    switch (e->input_mode) {
    case INPUT_NORMAL:
        x = e->window->edit_x + e->view->cx_display - e->view->vx;
        y = e->window->edit_y + e->view->cy - e->view->vy;
        break;
    case INPUT_COMMAND:
    case INPUT_SEARCH:
        x = e->cmdline_x;
        y = e->terminal.height - 1;
        break;
    default:
        BUG("unhandled input mode");
    }
    term_move_cursor(&e->terminal.obuf, x, y);
}

static void start_update(Terminal *term)
{
    term_begin_sync_update(term);
    term_hide_cursor(term);
}

static void clear_update_tabbar(Window *window, void* UNUSED_ARG(data))
{
    window->update_tabbar = false;
}

static void end_update(EditorState *e)
{
    Terminal *term = &e->terminal;
    restore_cursor(e);
    term_show_cursor(term);
    term_end_sync_update(term);
    term_output_flush(&term->obuf);

    e->buffer->changed_line_min = LONG_MAX;
    e->buffer->changed_line_max = -1;
    frame_for_each_window(e->root_frame, clear_update_tabbar, NULL);
}

static void update_all_windows(EditorState *e)
{
    update_window_sizes(&e->terminal, e->root_frame);
    frame_for_each_window(e->root_frame, update_window_full, NULL);
    update_separators(&e->terminal, &e->colors, e->root_frame);
}

static void update_window(EditorState *e, Window *window)
{
    if (e->options.tab_bar && window->update_tabbar) {
        print_tabbar(&e->terminal, &e->colors, window);
    }

    const View *view = window->view;
    if (e->options.show_line_numbers) {
        // Force updating lines numbers if all lines changed
        bool force = (view->buffer->changed_line_max == LONG_MAX);
        update_line_numbers(&e->terminal, &e->colors, window, force);
    }

    long y1 = MAX(view->buffer->changed_line_min, view->vy);
    long y2 = MIN(view->buffer->changed_line_max, view->vy + window->edit_h - 1);
    update_range(e, view, y1, y2 + 1);
    update_status_line(window);
}

// Update all visible views containing this buffer
static void update_buffer_windows(EditorState *e, const Buffer *buffer)
{
    for (size_t i = 0, n = buffer->views.count; i < n; i++) {
        View *view = buffer->views.ptrs[i];
        if (view->window->view == view) {
            // Visible view
            if (view != e->window->view) {
                // Restore cursor
                view->cursor.blk = BLOCK(view->buffer->blocks.next);
                block_iter_goto_offset(&view->cursor, view->saved_cursor_offset);

                // These have already been updated for current view
                view_update_cursor_x(view);
                view_update_cursor_y(view);
                view_update(view, e->options.scroll_margin);
            }
            update_window(e, view->window);
        }
    }
}

void normal_update(EditorState *e)
{
    Terminal *term = &e->terminal;
    start_update(term);
    update_term_title(term, e->buffer, e->options.set_window_title);
    update_all_windows(e);
    update_command_line(e);
    update_cursor_style(e);
    end_update(e);
}

static void ui_resize(EditorState *e)
{
    if (e->status == EDITOR_INITIALIZING) {
        return;
    }
    resized = 0;
    update_screen_size(&e->terminal, e->root_frame);
    normal_update(e);
}

void ui_start(EditorState *e)
{
    if (e->status == EDITOR_INITIALIZING) {
        return;
    }

    // Note: the order of these calls is important - Kitty saves/restores
    // some terminal state when switching buffers, so switching to the
    // alternate screen buffer needs to happen before modes are enabled
    term_use_alt_screen_buffer(&e->terminal);
    term_enable_private_modes(&e->terminal);

    ui_resize(e);
}

void ui_end(EditorState *e)
{
    if (e->status == EDITOR_INITIALIZING) {
        return;
    }
    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;
    term_clear_screen(obuf);
    term_move_cursor(obuf, 0, term->height - 1);
    term_restore_cursor_style(term);
    term_show_cursor(term);
    term_restore_private_modes(term);
    term_use_normal_screen_buffer(term);
    term_end_sync_update(term);
    term_output_flush(obuf);
    term_cooked();
}

static char get_choice(Terminal *term, const char *choices, unsigned int esc_timeout)
{
    KeyCode key = term_read_key(term, esc_timeout);
    if (key == KEY_NONE) {
        return 0;
    }

    switch (key) {
    case KEY_BRACKETED_PASTE:
    case KEY_DETECTED_PASTE:
        term_discard_paste(&term->ibuf, key == KEY_BRACKETED_PASTE);
        return 0;
    case MOD_CTRL | 'c':
    case MOD_CTRL | 'g':
    case MOD_CTRL | '[':
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

static void show_dialog (
    EditorState *e,
    const TermColor *text_color,
    const char *question
) {
    Terminal *term = &e->terminal;
    unsigned int question_width = u_str_width(question);
    unsigned int min_width = question_width + 2;
    if (term->height < 12 || term->width < min_width) {
        return;
    }

    unsigned int height = term->height / 4;
    unsigned int mid = term->height / 2;
    unsigned int top = mid - (height / 2);
    unsigned int bot = top + height;
    unsigned int width = MAX(term->width / 2, min_width);
    unsigned int x = (term->width - width) / 2;

    // The "underline" and "strikethrough" attributes should only apply
    // to the text, not the whole dialog background:
    TermColor dialog_color = *text_color;
    TermOutputBuffer *obuf = &term->obuf;
    dialog_color.attr &= ~(ATTR_UNDERLINE | ATTR_STRIKETHROUGH);
    set_color(term, &e->colors, &dialog_color);

    for (unsigned int y = top; y < bot; y++) {
        term_output_reset(term, x, width, 0);
        term_move_cursor(obuf, x, y);
        if (y == mid) {
            term_set_bytes(term, ' ', (width - question_width) / 2);
            set_color(term, &e->colors, text_color);
            term_add_str(obuf, question);
            set_color(term, &e->colors, &dialog_color);
        }
        term_clear_eol(term);
    }
}

char dialog_prompt(EditorState *e, const char *question, const char *choices)
{
    const TermColor *color = &e->colors.builtin[BC_DIALOG];
    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;

    normal_update(e);
    term_hide_cursor(term);
    show_dialog(e, color, question);
    show_message(term, &e->colors, question, false);
    term_output_flush(obuf);

    unsigned int esc_timeout = e->options.esc_timeout;
    char choice;
    while ((choice = get_choice(term, choices, esc_timeout)) == 0) {
        if (!resized) {
            continue;
        }
        ui_resize(e);
        term_hide_cursor(term);
        show_dialog(e, color, question);
        show_message(term, &e->colors, question, false);
        term_output_flush(obuf);
    }

    mark_everything_changed(e);
    return (choice >= 'a') ? choice : 0;
}

char status_prompt(EditorState *e, const char *question, const char *choices)
{
    // update_windows() assumes these have been called for the current view
    view_update_cursor_x(e->view);
    view_update_cursor_y(e->view);
    view_update(e->view, e->options.scroll_margin);

    // Set changed_line_min and changed_line_max before calling update_range()
    mark_all_lines_changed(e->buffer);

    Terminal *term = &e->terminal;
    start_update(term);
    update_term_title(term, e->buffer, e->options.set_window_title);
    update_buffer_windows(e, e->buffer);
    show_message(term, &e->colors, question, false);
    end_update(e);

    unsigned int esc_timeout = e->options.esc_timeout;
    char choice;
    while ((choice = get_choice(term, choices, esc_timeout)) == 0) {
        if (!resized) {
            continue;
        }
        ui_resize(e);
        term_hide_cursor(term);
        show_message(term, &e->colors, question, false);
        restore_cursor(e);
        term_show_cursor(term);
        term_output_flush(&term->obuf);
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

static void update_screen(EditorState *e, const ScreenState *s)
{
    if (e->everything_changed) {
        normal_update(e);
        e->everything_changed = false;
        return;
    }

    Buffer *buffer = e->buffer;
    View *view = e->view;
    view_update_cursor_x(view);
    view_update_cursor_y(view);
    view_update(view, e->options.scroll_margin);

    if (s->id == buffer->id) {
        if (s->vx != view->vx || s->vy != view->vy) {
            mark_all_lines_changed(buffer);
        } else {
            // Because of trailing whitespace highlighting and highlighting
            // current line in different color, the lines cy (old cursor y) and
            // view->cy need to be updated. Always update at least current line.
            buffer_mark_lines_changed(buffer, s->cy, view->cy);
        }
        if (s->is_modified != buffer_modified(buffer)) {
            mark_buffer_tabbars_changed(buffer);
        }
    } else {
        e->window->update_tabbar = true;
        mark_all_lines_changed(buffer);
    }

    start_update(&e->terminal);
    if (e->window->update_tabbar) {
        update_term_title(&e->terminal, e->buffer, e->options.set_window_title);
    }
    update_buffer_windows(e, buffer);
    update_command_line(e);
    if (e->cursor_style_changed) {
        update_cursor_style(e);
    }
    end_update(e);
}

int main_loop(EditorState *e)
{
    while (e->status == EDITOR_RUNNING) {
        if (unlikely(resized)) {
            LOG_INFO("SIGWINCH received");
            ui_resize(e);
        }

        KeyCode key = term_read_key(&e->terminal, e->options.esc_timeout);
        if (unlikely(key == KEY_NONE)) {
            continue;
        }

        const ScreenState s = {
            .is_modified = buffer_modified(e->buffer),
            .id = e->buffer->id,
            .cy = e->view->cy,
            .vx = e->view->vx,
            .vy = e->view->vy
        };

        clear_error();
        handle_input(e, key);
        sanity_check(e->view);
        update_screen(e, &s);
    }

    BUG_ON(e->status < 0 || e->status > EDITOR_EXIT_MAX);
    return e->status;
}
