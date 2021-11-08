#include <stdlib.h>
#include <string.h>
#include "cmdline.h"
#include "completion.h"
#include "editor.h"
#include "history.h"
#include "search.h"
#include "terminal/input.h"
#include "util/ascii.h"
#include "util/bsearch.h"
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

static void set_text(CommandLine *c, const char *text)
{
    string_clear(&c->buf);
    const size_t text_len = strlen(text);
    string_append_buf(&c->buf, text, text_len);
    c->pos = text_len;
}

void cmdline_set_text(CommandLine *c, const char *text)
{
    set_text(c, text);
    c->search_pos = NULL;
}

static void cmd_bol(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    e->cmdline.pos = 0;
    reset_completion(&e->cmdline);
}

static void cmd_cancel(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    cmdline_clear(c);
    set_input_mode(INPUT_NORMAL);
    reset_completion(c);
}

static void cmd_clear(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    cmdline_clear(&e->cmdline);
}

static void cmd_delete(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    cmdline_delete(c);
    c->search_pos = NULL;
    reset_completion(c);
}

static void cmd_delete_eol(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    c->buf.len = c->pos;
    c->search_pos = NULL;
    reset_completion(c);
}

static void cmd_delete_word(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    const unsigned char *buf = c->buf.buffer;
    const size_t len = c->buf.len;
    size_t i = c->pos;

    if (i == len) {
        return;
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
}

static void cmd_eol(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    c->pos = c->buf.len;
    reset_completion(c);
}

static void cmd_erase(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    if (c->pos > 0) {
        u_prev_char(c->buf.buffer, &c->pos);
        cmdline_delete(c);
    }
    c->search_pos = NULL;
    reset_completion(c);
}

static void cmd_erase_bol(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    string_remove(&c->buf, 0, c->pos);
    c->pos = 0;
    c->search_pos = NULL;
    reset_completion(c);
}

static void cmd_erase_word(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    size_t i = c->pos;
    if (i == 0) {
        return;
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
}

static const History *get_history(EditorState *e)
{
    switch (e->input_mode) {
    case INPUT_COMMAND:
        return &e->command_history;
    case INPUT_SEARCH:
        return &e->search_history;
    case INPUT_NORMAL:
        return NULL;
    }
    BUG("unhandled input mode");
    return NULL;
}

static void cmd_history_prev(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    const History *hist = get_history(e);
    if (unlikely(!hist)) {
        return;
    }

    CommandLine *c = &e->cmdline;
    if (!c->search_pos) {
        free(c->search_text);
        c->search_text = string_clone_cstring(&c->buf);
    }

    if (history_search_forward(hist, &c->search_pos, c->search_text)) {
        set_text(c, c->search_pos->text);
    }

    reset_completion(c);
}

static void cmd_history_next(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    const History *hist = get_history(e);
    if (unlikely(!hist)) {
        return;
    }

    CommandLine *c = &e->cmdline;
    if (!c->search_pos) {
        goto out;
    }

    if (history_search_backward(hist, &c->search_pos, c->search_text)) {
        set_text(c, c->search_pos->text);
    } else {
        set_text(c, c->search_text);
        c->search_pos = NULL;
    }

out:
    reset_completion(c);
}

static void cmd_left(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    if (c->pos) {
        u_prev_char(c->buf.buffer, &c->pos);
    }
    reset_completion(c);
}

static void cmd_right(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    if (c->pos < c->buf.len) {
        u_get_char(c->buf.buffer, c->buf.len, &c->pos);
    }
    reset_completion(c);
}

static void cmd_word_bwd(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    CommandLine *c = &e->cmdline;
    if (c->pos <= 1) {
        c->pos = 0;
        return;
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
}

static void cmd_word_fwd(const CommandArgs *a)
{
    EditorState *e = a->userdata;
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
}

static void cmd_complete_next(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    complete_command_next(&e->cmdline);
}

static void cmd_complete_prev(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    complete_command_prev(&e->cmdline);
}

static void cmd_case(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    unsigned int *css = &e->options.case_sensitive_search;
    *css = (*css + 1) % 3;
}

static void cmd_direction(const CommandArgs *a)
{
    EditorState *e = a->userdata;
    toggle_search_direction(&e->search.direction);
}

static const Command common_cmds[] = {
    {"bol", "", false, 0, 0, cmd_bol},
    {"cancel", "", false, 0, 0, cmd_cancel},
    {"clear", "", false, 0, 0, cmd_clear},
    {"delete", "", false, 0, 0, cmd_delete},
    {"delete-eol", "", false, 0, 0, cmd_delete_eol},
    {"delete-word", "", false, 0, 0, cmd_delete_word},
    {"eol", "", false, 0, 0, cmd_eol},
    {"erase", "", false, 0, 0, cmd_erase},
    {"erase-bol", "", false, 0, 0, cmd_erase_bol},
    {"erase-word", "", false, 0, 0, cmd_erase_word},
    {"history-next", "", false, 0, 0, cmd_history_next},
    {"history-prev", "", false, 0, 0, cmd_history_prev},
    {"left", "", false, 0, 0, cmd_left},
    {"right", "", false, 0, 0, cmd_right},
    {"word-bwd", "", false, 0, 0, cmd_word_bwd},
    {"word-fwd", "", false, 0, 0, cmd_word_fwd},
};

static const Command search_cmds[] = {
    {"case", "", false, 0, 0, cmd_case},
    {"direction", "", false, 0, 0, cmd_direction},
};

static const Command command_cmds[] = {
    {"complete-next", "", false, 0, 0, cmd_complete_next},
    {"complete-prev", "", false, 0, 0, cmd_complete_prev},
};

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
    .lookup = find_cmd_mode_command,
    .allow_recording = NULL,
    .aliases = HASHMAP_INIT,
    .userdata = &editor,
};

const CommandSet search_mode_commands = {
    .lookup = find_search_mode_command,
    .allow_recording = NULL,
    .aliases = HASHMAP_INIT,
    .userdata = &editor,
};
