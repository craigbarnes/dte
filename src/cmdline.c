#include <stdlib.h>
#include <string.h>
#include "cmdline.h"
#include "command/args.h"
#include "command/macro.h"
#include "commands.h"
#include "completion.h"
#include "copy.h"
#include "editor.h"
#include "history.h"
#include "options.h"
#include "search.h"
#include "terminal/osc52.h"
#include "util/ascii.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/utf8.h"

static void cmdline_delete(CommandLine *c)
{
    size_t pos = c->pos;
    size_t len = 1;

    if (pos == c->buf.len) {
        return;
    }

    u_get_char(c->buf.buffer, c->buf.len, &pos);
    len = pos - c->pos;
    string_remove(&c->buf, c->pos, len);
}

void cmdline_clear(CommandLine *c)
{
    string_clear(&c->buf);
    c->pos = 0;
    c->search_pos = NULL;
}

void cmdline_free(CommandLine *c)
{
    cmdline_clear(c);
    string_free(&c->buf);
    free(c->search_text);
    reset_completion(c);
}

static void set_text(CommandLine *c, const char *text)
{
    string_clear(&c->buf);
    const size_t text_len = strlen(text);
    c->pos = text_len;
    string_append_buf(&c->buf, text, text_len);
}

void cmdline_set_text(CommandLine *c, const char *text)
{
    c->search_pos = NULL;
    set_text(c, text);
}

static bool cmd_bol(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    e->cmdline.pos = 0;
    reset_completion(&e->cmdline);
    return true;
}

static bool cmd_cancel(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    cmdline_clear(c);
    set_input_mode(e, INPUT_NORMAL);
    reset_completion(c);
    return true;
}

static bool cmd_clear(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    cmdline_clear(&e->cmdline);
    return true;
}

static bool cmd_copy(EditorState *e, const CommandArgs *a)
{
    bool internal = cmdargs_has_flag(a, 'i') || a->flag_set == 0;
    bool clipboard = cmdargs_has_flag(a, 'b');
    bool primary = cmdargs_has_flag(a, 'p');

    String *buf = &e->cmdline.buf;
    size_t len = buf->len;
    if (internal) {
        char *str = string_clone_cstring(buf);
        record_copy(&e->clipboard, str, len, false);
    }

    Terminal *term = &e->terminal;
    if ((clipboard || primary) && term->features & TFLAG_OSC52_COPY) {
        const char *str = string_borrow_cstring(buf);
        if (!term_osc52_copy(&term->obuf, str, len, clipboard, primary)) {
            LOG_ERRNO("term_osc52_copy");
            // TODO: return false ?
        }
    }

    return true;
}

static bool cmd_delete(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    cmdline_delete(c);
    c->search_pos = NULL;
    reset_completion(c);
    return true;
}

static bool cmd_delete_eol(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    c->buf.len = c->pos;
    c->search_pos = NULL;
    reset_completion(c);
    return true;
}

static bool cmd_delete_word(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    const unsigned char *buf = c->buf.buffer;
    const size_t len = c->buf.len;
    size_t i = c->pos;

    if (i == len) {
        return true;
    }

    while (i < len && is_word_byte(buf[i])) {
        i++;
    }

    while (i < len && !is_word_byte(buf[i])) {
        i++;
    }

    string_remove(&c->buf, c->pos, i - c->pos);

    c->search_pos = NULL;
    reset_completion(c);
    return true;
}

static bool cmd_eol(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    c->pos = c->buf.len;
    reset_completion(c);
    return true;
}

static bool cmd_erase(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    if (c->pos > 0) {
        u_prev_char(c->buf.buffer, &c->pos);
        cmdline_delete(c);
    }
    c->search_pos = NULL;
    reset_completion(c);
    return true;
}

static bool cmd_erase_bol(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    string_remove(&c->buf, 0, c->pos);
    c->pos = 0;
    c->search_pos = NULL;
    reset_completion(c);
    return true;
}

static bool cmd_erase_word(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    size_t i = c->pos;
    if (i == 0) {
        return true;
    }

    // open /path/to/file^W => open /path/to/

    // erase whitespace
    while (i && ascii_isspace(c->buf.buffer[i - 1])) {
        i--;
    }

    // erase non-word bytes
    while (i && !is_word_byte(c->buf.buffer[i - 1])) {
        i--;
    }

    // erase word bytes
    while (i && is_word_byte(c->buf.buffer[i - 1])) {
        i--;
    }

    string_remove(&c->buf, i, c->pos - i);
    c->pos = i;
    c->search_pos = NULL;
    reset_completion(c);
    return true;
}

static bool do_history_prev(const History *hist, CommandLine *c)
{
    if (!c->search_pos) {
        free(c->search_text);
        c->search_text = string_clone_cstring(&c->buf);
    }

    if (history_search_forward(hist, &c->search_pos, c->search_text)) {
        BUG_ON(!c->search_pos);
        set_text(c, c->search_pos->text);
    }

    reset_completion(c);
    return true;
}

static bool do_history_next(const History *hist, CommandLine *c)
{
    if (!c->search_pos) {
        goto out;
    }

    if (history_search_backward(hist, &c->search_pos, c->search_text)) {
        BUG_ON(!c->search_pos);
        set_text(c, c->search_pos->text);
    } else {
        set_text(c, c->search_text);
        c->search_pos = NULL;
    }

out:
    reset_completion(c);
    return true;
}

static bool cmd_search_history_next(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    return do_history_next(&e->search_history, &e->cmdline);
}

static bool cmd_search_history_prev(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    return do_history_prev(&e->search_history, &e->cmdline);
}

static bool cmd_command_history_next(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    return do_history_next(&e->command_history, &e->cmdline);
}

static bool cmd_command_history_prev(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    return do_history_prev(&e->command_history, &e->cmdline);
}

static bool cmd_left(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    if (c->pos) {
        u_prev_char(c->buf.buffer, &c->pos);
    }
    reset_completion(c);
    return true;
}

static bool cmd_paste(EditorState *e, const CommandArgs *a)
{
    CommandLine *c = &e->cmdline;
    const Clipboard *clip = &e->clipboard;
    string_insert_buf(&c->buf, c->pos, clip->buf, clip->len);
    if (cmdargs_has_flag(a, 'm')) {
        c->pos += clip->len;
    }
    c->search_pos = NULL;
    reset_completion(c);
    return true;
}

static bool cmd_right(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    if (c->pos < c->buf.len) {
        u_get_char(c->buf.buffer, c->buf.len, &c->pos);
    }
    reset_completion(c);
    return true;
}

static bool cmd_toggle(EditorState *e, const CommandArgs *a)
{
    const char *option_name = a->args[0];
    bool global = cmdargs_has_flag(a, 'g');
    size_t nr_values = a->nr_args - 1;
    if (nr_values == 0) {
        return toggle_option(e, option_name, global, false);
    }

    char **values = a->args + 1;
    return toggle_option_values(e, option_name, global, false, values, nr_values);
}

static bool cmd_word_bwd(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    if (c->pos <= 1) {
        c->pos = 0;
        return true;
    }

    const unsigned char *const buf = c->buf.buffer;
    size_t i = c->pos - 1;

    while (i > 0 && !is_word_byte(buf[i])) {
        i--;
    }

    while (i > 0 && is_word_byte(buf[i])) {
        i--;
    }

    if (i > 0) {
        i++;
    }

    c->pos = i;
    reset_completion(c);
    return true;
}

static bool cmd_word_fwd(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    const unsigned char *buf = c->buf.buffer;
    const size_t len = c->buf.len;
    size_t i = c->pos;

    while (i < len && is_word_byte(buf[i])) {
        i++;
    }

    while (i < len && !is_word_byte(buf[i])) {
        i++;
    }

    c->pos = i;
    reset_completion(c);
    return true;
}

static bool cmd_complete_next(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    complete_command_next(e);
    return true;
}

static bool cmd_complete_prev(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    complete_command_prev(e);
    return true;
}

static bool cmd_direction(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    toggle_search_direction(&e->search);
    return true;
}

static bool cmd_command_mode_accept(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    CommandLine *c = &e->cmdline;
    reset_completion(c);
    set_input_mode(e, INPUT_NORMAL);

    const char *str = string_borrow_cstring(&c->buf);
    cmdline_clear(c);
    if (!cmdargs_has_flag(a, 'H') && str[0] != ' ') {
        // This is done before handle_command() because "command [text]"
        // can modify the contents of the command-line
        history_add(&e->command_history, str);
    }

    current_command = NULL;
    return handle_normal_command(e, str, true);
}

static bool cmd_search_mode_accept(EditorState *e, const CommandArgs *a)
{
    CommandLine *c = &e->cmdline;
    if (cmdargs_has_flag(a, 'e')) {
        if (c->buf.len == 0) {
            return true;
        }
        // Escape the regex; to match as plain text
        char *original = string_clone_cstring(&c->buf);
        size_t len = c->buf.len;
        string_clear(&c->buf);
        for (size_t i = 0; i < len; i++) {
            char ch = original[i];
            if (is_regex_special_char(ch)) {
                string_append_byte(&c->buf, '\\');
            }
            string_append_byte(&c->buf, ch);
        }
        free(original);
    }

    const char *str = NULL;
    bool add_to_history = !cmdargs_has_flag(a, 'H');
    if (c->buf.len > 0) {
        str = string_borrow_cstring(&c->buf);
        BUG_ON(!str);
        search_set_regexp(&e->search, str);
        if (add_to_history) {
            history_add(&e->search_history, str);
        }
    }

    if (e->macro.recording) {
        const char *args[5];
        size_t i = 0;
        if (str) {
            if (e->search.reverse) {
                args[i++] = "-r";
            }
            if (!add_to_history) {
                args[i++] = "-H";
            }
            if (unlikely(str[0] == '-')) {
                args[i++] = "--";
            }
            args[i++] = str;
        } else {
            args[i++] = e->search.reverse ? "-p" : "-n";
        }
        args[i] = NULL;
        macro_command_hook(&e->macro, "search", (char**)args);
    }

    current_command = NULL;
    bool found = search_next(e->view, &e->search, e->options.case_sensitive_search);
    cmdline_clear(c);
    set_input_mode(e, INPUT_NORMAL);
    return found;
}

IGNORE_WARNING("-Wincompatible-pointer-types")

static const Command common_cmds[] = {
    {"bol", "", false, 0, 0, cmd_bol},
    {"cancel", "", false, 0, 0, cmd_cancel},
    {"clear", "", false, 0, 0, cmd_clear},
    {"copy", "bip", false, 0, 0, cmd_copy},
    {"delete", "", false, 0, 0, cmd_delete},
    {"delete-eol", "", false, 0, 0, cmd_delete_eol},
    {"delete-word", "", false, 0, 0, cmd_delete_word},
    {"eol", "", false, 0, 0, cmd_eol},
    {"erase", "", false, 0, 0, cmd_erase},
    {"erase-bol", "", false, 0, 0, cmd_erase_bol},
    {"erase-word", "", false, 0, 0, cmd_erase_word},
    {"left", "", false, 0, 0, cmd_left},
    {"paste", "m", false, 0, 0, cmd_paste},
    {"right", "", false, 0, 0, cmd_right},
    {"toggle", "g", false, 1, -1, cmd_toggle},
    {"word-bwd", "", false, 0, 0, cmd_word_bwd},
    {"word-fwd", "", false, 0, 0, cmd_word_fwd},
};

static const Command search_cmds[] = {
    {"accept", "eH", false, 0, 0, cmd_search_mode_accept},
    {"direction", "", false, 0, 0, cmd_direction},
    {"history-next", "", false, 0, 0, cmd_search_history_next},
    {"history-prev", "", false, 0, 0, cmd_search_history_prev},
};

static const Command command_cmds[] = {
    {"accept", "H", false, 0, 0, cmd_command_mode_accept},
    {"complete-next", "", false, 0, 0, cmd_complete_next},
    {"complete-prev", "", false, 0, 0, cmd_complete_prev},
    {"history-next", "", false, 0, 0, cmd_command_history_next},
    {"history-prev", "", false, 0, 0, cmd_command_history_prev},
};

UNIGNORE_WARNINGS

static const Command *find_cmd_mode_command(const char *name)
{
    const Command *cmd = BSEARCH(name, common_cmds, command_cmp);
    return cmd ? cmd : BSEARCH(name, command_cmds, command_cmp);
}

static const Command *find_search_mode_command(const char *name)
{
    const Command *cmd = BSEARCH(name, common_cmds, command_cmp);
    return cmd ? cmd : BSEARCH(name, search_cmds, command_cmp);
}

const CommandSet cmd_mode_commands = {
    .lookup = find_cmd_mode_command
};

const CommandSet search_mode_commands = {
    .lookup = find_search_mode_command
};
