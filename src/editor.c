#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "editor.h"
#include "bind.h"
#include "bookmark.h"
#include "compiler.h"
#include "encoding.h"
#include "error.h"
#include "file-option.h"
#include "filetype.h"
#include "lock.h"
#include "signals.h"
#include "syntax/syntax.h"
#include "terminal/color.h"
#include "terminal/input.h"
#include "terminal/key.h"
#include "terminal/output.h"
#include "terminal/paste.h"
#include "ui.h"
#include "util/exitcode.h"
#include "util/intern.h"
#include "util/intmap.h"
#include "util/log.h"
#include "util/xmalloc.h"
#include "util/xstdio.h"
#include "version.h"

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
        LOG_NOTICE("using fallback locale for LC_CTYPE: %s", fallback);
        return;
    }

    LOG_ERROR("no UTF-8 fallback locales found");
    fputs("setlocale() failed\n", stderr);
    exit(EC_CONFIG_ERROR);
}

static mode_t get_umask(void)
{
    mode_t old = umask(0);
    umask(old);
    return old;
}

EditorState *init_editor_state(void)
{
    EditorState *e = xnew(EditorState, 1);
    *e = (EditorState) {
        .status = EDITOR_INITIALIZING,
        .version = VERSION,
        .command_history = {
            .max_entries = 512,
        },
        .search_history = {
            .max_entries = 128,
        },
        .cursor_styles = {
            [CURSOR_MODE_DEFAULT] = {.type = CURSOR_DEFAULT, .color = COLOR_DEFAULT},
            [CURSOR_MODE_INSERT] = {.type = CURSOR_KEEP, .color = COLOR_KEEP},
            [CURSOR_MODE_OVERWRITE] = {.type = CURSOR_KEEP, .color = COLOR_KEEP},
            [CURSOR_MODE_CMDLINE] = {.type = CURSOR_KEEP, .color = COLOR_KEEP},
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
            .save_unmodified = SAVE_FULL,
            .syntax = true,
            .tab_width = 8,
            .text_width = 72,
            .ws_error = WSE_SPECIAL,

            // Global-only options
            .case_sensitive_search = CSS_TRUE,
            .crlf_newlines = false,
            .display_special = false,
            .esc_timeout = 100,
            .filesize_limit = 250,
            .lock_files = true,
            .optimize_true_color = true,
            .scroll_margin = 0,
            .select_cursor_char = true,
            .set_window_title = false,
            .show_line_numbers = false,
            .statusline_left = str_intern(" %f%s%m%s%r%s%M"),
            .statusline_right = str_intern(" %y,%X  %u  %o  %E%s%b%s%n %t   %p "),
            .tab_bar = true,
            .utf8_bom = false,
            .window_separator = WINSEP_BAR,
        }
    };

    sanity_check_global_options(&e->options);

    const char *home = getenv("HOME");
    const char *dte_home = getenv("DTE_HOME");
    home = home ? home : "";
    e->home_dir = strview_intern(home);
    if (dte_home) {
        e->user_config_dir = xstrdup(dte_home);
    } else {
        e->user_config_dir = xstrjoin(home, "/.dte");
    }

    pid_t pid = getpid();
    bool leader = pid == getsid(0);
    e->session_leader = leader;
    LOG_INFO("pid: %jd%s", (intmax_t)pid, leader ? " (session leader)" : "");

    pid_t pgid = getpgrp();
    if (pgid != pid) {
        LOG_INFO("pgid: %jd", (intmax_t)pgid);
    }

    mode_t mask = get_umask();
    e->new_file_mode = 0666 & ~mask;
    LOG_INFO("umask: %04o", 0777u & (unsigned int)mask);

    set_and_check_locale();
    init_file_locks_context(&e->locks_ctx, e->user_config_dir, pid);

    // Allow child processes to detect that they're running under dte
    if (unlikely(setenv("DTE_VERSION", VERSION, 1) != 0)) {
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
    hashmap_init(&e->aliases, 32);
    hashset_init(&e->required_syntax_files, 0, false);
    hashset_init(&e->required_syntax_builtins, 0, false);

    HashMap *modes = &e->modes;
    e->normal_mode = new_mode(modes, xstrdup("normal"), &normal_commands);
    e->command_mode = new_mode(modes, xstrdup("command"), &cmd_mode_commands);
    e->search_mode = new_mode(modes, xstrdup("search"), &search_mode_commands);
    e->mode = e->normal_mode;
    intmap_init(&e->normal_mode->key_bindings, 150);
    intmap_init(&e->command_mode->key_bindings, 40);
    intmap_init(&e->search_mode->key_bindings, 40);

    return e;
}

static void free_mode_handler(ModeHandler *handler)
{
    ptr_array_free_array(&handler->fallthrough_modes);
    free_bindings(&handler->key_bindings);
    free(handler);
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
    free_buffers(&e->buffers, &e->locks_ctx);
    free_file_locks_context(&e->locks_ctx);

    ptr_array_free_cb(&e->bookmarks, FREE_FUNC(file_location_free));
    hashmap_free(&e->compilers, FREE_FUNC(free_compiler));
    hashmap_free(&e->modes, FREE_FUNC(free_mode_handler));
    hashmap_free(&e->styles.other, free);
    hashmap_free(&e->aliases, free);
    hashset_free(&e->required_syntax_files);
    hashset_free(&e->required_syntax_builtins);

    free_interned_strings();
    free_interned_regexps();

    // TODO: intern this (so that it's freed by free_interned_strings())
    free((void*)e->user_config_dir);

    free(e);
}

static bool buffer_contains_block(const Buffer *buffer, const Block *ref)
{
    const Block *blk;
    block_for_each(blk, &buffer->blocks) {
        if (blk == ref) {
            return true;
        }
    }
    return false;
}

static void sanity_check(const View *view)
{
    if (DEBUG < 1) {
        return;
    }
    const BlockIter *cursor = &view->cursor;
    BUG_ON(!buffer_contains_block(view->buffer, cursor->blk));
    BUG_ON(cursor->offset > cursor->blk->size);
}

void any_key(Terminal *term, unsigned int esc_timeout)
{
    KeyCode key;
    xfputs("Press any key to continue\r\n", stderr);
    while ((key = term_read_input(term, esc_timeout)) == KEY_NONE) {
        ;
    }
    bool bracketed_paste = key == KEYCODE_BRACKETED_PASTE;
    if (bracketed_paste || key == KEYCODE_DETECTED_PASTE) {
        term_discard_paste(&term->ibuf, bracketed_paste);
    }
}

NOINLINE
void ui_resize(EditorState *e)
{
    if (e->status == EDITOR_INITIALIZING) {
        return;
    }

    resized = 0;
    update_screen_size(&e->terminal, e->root_frame);

    const ScreenState dummyval = {.id = 0};
    e->screen_update |= UPDATE_ALL;
    update_screen(e, &dummyval);
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
}

int main_loop(EditorState *e)
{
    while (e->status == EDITOR_RUNNING) {
        if (unlikely(resized)) {
            LOG_INFO("SIGWINCH received");
            ui_resize(e);
        }

        KeyCode key = term_read_input(&e->terminal, e->options.esc_timeout);
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
