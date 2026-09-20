#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "exec.h"
#include "block-iter.h"
#include "buffer.h"
#include "change.h"
#include "commands.h"
#include "ctags.h"
#include "editor.h"
#include "move.h"
#include "msg.h"
#include "selection.h"
#include "spawn.h"
#include "tag.h"
#include "terminal/mode.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/string.h"
#include "util/strtonum.h"
#include "util/xsnprintf.h"
#include "view.h"
#include "window.h"

enum {
    IN = 1 << 0,
    OUT = 1 << 1,
    ERR = 1 << 2,
    ALL = IN | OUT | ERR,
};

static const struct {
    char name[11];
    uint8_t flags;
} exec_map[] = {
    [EXEC_BUFFER] = {"buffer", IN | OUT},
    [EXEC_COMMAND] = {"command", IN},
    [EXEC_ECHO] = {"echo", OUT},
    [EXEC_ERRMSG] = {"errmsg", ERR},
    [EXEC_EVAL] = {"eval", OUT},
    [EXEC_LINE] = {"line", IN},
    [EXEC_MSG] = {"msg", IN | OUT},
    [EXEC_MSG_A] = {"msgA", IN | OUT},
    [EXEC_MSG_B] = {"msgB", IN | OUT},
    [EXEC_MSG_C] = {"msgC", IN | OUT},
    [EXEC_NULL] = {"null", ALL},
    [EXEC_OPEN] = {"open", IN | OUT},
    [EXEC_OPEN_REL] = {"open-rel", IN},
    [EXEC_SEARCH] = {"search", IN},
    [EXEC_TAG] = {"tag", OUT},
    [EXEC_TAG_A] = {"tagA", OUT},
    [EXEC_TAG_B] = {"tagB", OUT},
    [EXEC_TAG_C] = {"tagC", OUT},
    [EXEC_TTY] = {"tty", ALL},
    [EXEC_WORD] = {"word", IN},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(exec_map, name);
}

ExecAction lookup_exec_action(const char *name, int fd)
{
    BUG_ON(fd < 0 || fd > 2);
    ssize_t i = BSEARCH_IDX(name, exec_map, vstrcmp);
    return (i >= 0 && (exec_map[i].flags & 1u << fd)) ? i : EXEC_INVALID;
}

void collect_exec_actions(PointerArray *a, StringView prefix, int fd)
{
    if (unlikely(fd < 0 || fd > 2)) {
        return;
    }

    unsigned int flag = 1u << fd;
    for (size_t i = 0; i < ARRAYLEN(exec_map); i++) {
        const char *action = exec_map[i].name;
        if ((exec_map[i].flags & flag) && str_has_sv_prefix(action, prefix)) {
            ptr_array_append(a, xstrdup(action));
        }
    }
}

static void open_files_from_string(EditorState *e, const String *str)
{
    PointerArray filenames = ptr_array_new(0);
    for (size_t pos = 0, size = str->len; pos < size; ) {
        char *filename = buf_next_line(str->buffer, &pos, size); // Mutates `str->buffer`
        if (filename[0] != '\0') {
            ptr_array_append(&filenames, filename);
        }
    }

    if (filenames.count == 0) {
        return;
    }

    ptr_array_append(&filenames, NULL);
    window_open_files(e->window, (char**)filenames.ptrs, NULL);

    // TODO: re-enable this when the todo in allow_macro_recording() is done
    // macro_command_hook(&e->macro, "open", (char**)filenames.ptrs);

    ptr_array_free_array(&filenames);
}

static void parse_and_activate_message(EditorState *e, StringView str, ExecAction action)
{
    if (unlikely(str.length == 0)) {
        error_msg(&e->err, "child produced no output");
        return;
    }

    size_t msgs_idx = (action == EXEC_MSG) ? 0 : action - EXEC_MSG_A;
    BUG_ON(msgs_idx >= ARRAYLEN(e->messages));
    MessageList *msgs = &e->messages[msgs_idx];
    size_t count = msgs->array.count;
    size_t x;

    if (!count || !buf_parse_size(str, &x) || !x) {
        return;
    }

    msgs->pos = MIN(x - 1, count - 1);
    activate_current_message(msgs, e->window, &e->err);
}

static void parse_and_activate_tags(EditorState *e, StringView str, ExecAction action)
{
    ErrorBuffer *ebuf = &e->err;
    if (unlikely(str.length == 0)) {
        error_msg(ebuf, "child produced no output");
        return;
    }

    char cwd[8192];
    if (unlikely(!getcwd(cwd, sizeof cwd))) {
        error_msg_errno(ebuf, "getcwd() failed");
        return;
    }

    const StringView dir = strview(cwd);
    const char *buffer_filename = e->buffer->abs_filename;
    TagFile *tf = &e->tagfile;
    enum {NOT_LOADED, LOADED, FAILED} tf_status = NOT_LOADED;

    size_t msgs_idx = (action == EXEC_TAG) ? e->options.msg_tag : action - EXEC_TAG_A;
    BUG_ON(msgs_idx >= ARRAYLEN(e->messages));
    MessageList *msgs = &e->messages[msgs_idx];
    clear_messages(msgs);

    for (size_t pos = 0; pos < str.length; ) {
        Tag tag;
        StringView line = buf_slice_next_line(str.data, &pos, str.length);
        if (line.length == 0) {
            continue;
        }

        bool parsed = parse_ctags_line(&tag, line);
        if (parsed) {
            // `line` is a valid tags(5) file entry; handle it directly
            add_message_for_tag(msgs, &tag, dir);
            continue;
        }

        // Treat `line` as a simple tag name (look it up in the tags(5) file)
        switch (tf_status) {
        case NOT_LOADED:
            tf_status = load_tag_file(tf, ebuf) ? LOADED : FAILED;
            if (tf_status == FAILED) {
                continue;
            }
            // Fallthrough
        case LOADED:
            tag_lookup(tf, msgs, ebuf, line, buffer_filename);
            continue;
        case FAILED:
            continue;
        }
    }

    activate_current_message_save(msgs, &e->bookmarks, e->view, ebuf);
}

static void insert_to_selection (
    View *view,
    StringView text,
    const SelectionInfo *info
) {
    size_t del_count = info->eo - info->so;
    buffer_replace_bytes(view, del_count, text);

    if (text.length == 0) {
        // If the selection was replaced with 0 bytes then there's nothing
        // new to select, so just unselect instead
        unselect(view);
        return;
    }

    // Keep the selection and adjust the size to the newly inserted text
    size_t so = info->so;
    size_t eo = so + (text.length - 1);
    block_iter_goto_offset(&view->cursor, info->swapped ? so : eo);
    view->sel_so = info->swapped ? eo : so;
    view->sel_eo = SEL_EO_RECALC;
}

static void show_spawn_error_msg(ErrorBuffer *ebuf, StringView errstr, int err)
{
    if (err <= 0) {
        // spawn() returned an error code instead of an exit code, which
        // indicates that something failed before (or during) the child
        // process exec(3p), or that an error occurred in wait_child().
        // In this case, error_msg() has already been called.
        return;
    }

    char msg[512];
    msg[0] = '\0';
    if (errstr.length) {
        size_t pos = 0;
        StringView line = buf_slice_next_line(errstr.data, &pos, errstr.length);
        BUG_ON(pos == 0);
        size_t len = MIN(line.length, sizeof(msg) - 8);
        xsnprintf(msg, sizeof(msg), ": \"%.*s\"", (int)len, line.data);
    }

    if (err >= 256) {
        int sig = err >> 8;
        const char *str = strsignal(sig);
        error_msg(ebuf, "Child received signal %d (%s)%s", sig, str ? str : "??", msg);
    } else if (err) {
        error_msg(ebuf, "Child returned %d%s", err, msg);
    }
}

static size_t msgs_idx_from_exec_action(ExecAction action)
{
    BUG_ON(action < EXEC_MSG || action > EXEC_MSG_C);
    return (action == EXEC_MSG) ? 0 : action - EXEC_MSG_A;
}

static SpawnAction spawn_action_from_exec_action(ExecAction action)
{
    BUG_ON(action == EXEC_INVALID);
    if (action == EXEC_NULL) {
        return SPAWN_NULL;
    } else if (action == EXEC_TTY) {
        return SPAWN_TTY;
    } else {
        return SPAWN_PIPE;
    }
}

// NOLINTNEXTLINE(readability-function-size)
ssize_t handle_exec (
    EditorState *e,
    const char **argv,
    ExecAction actions[3],
    ExecFlags exec_flags
) {
    View *view = e->view;
    const BlockIter saved_cursor = view->cursor;
    const ssize_t saved_sel_so = view->sel_so;
    const ssize_t saved_sel_eo = view->sel_eo;
    String input = string_new(0);
    size_t input_from_buffer_length = 0;
    bool input_from_buffer = false;
    bool output_to_buffer = (actions[STDOUT_FILENO] == EXEC_BUFFER);
    bool quiet = (exec_flags & EXECFLAG_QUIET);

    SpawnContext ctx = {
        .argv = argv,
        .outputs = {string_new(0), string_new(0)},
        .quiet = quiet,
        .ebuf = &e->err,
        .lines = output_to_buffer ? view->window->edit_h : 0,
        .columns = output_to_buffer ? view->window->edit_w : 0,
        .actions = {
            spawn_action_from_exec_action(actions[0]),
            spawn_action_from_exec_action(actions[1]),
            spawn_action_from_exec_action(actions[2]),
        },
    };

    ExecAction in_action = actions[STDIN_FILENO];
    switch (in_action) {
    case EXEC_LINE:
        input_from_buffer = true;
        if (!view->selection) {
            move_bol(view, BOL_SIMPLE);
            input_from_buffer_length = block_iter_get_line(&view->cursor).length;
        }
        break;
    case EXEC_BUFFER:
        input_from_buffer = true;
        if (!view->selection) {
            const Block *blk;
            block_for_each(blk, &view->buffer->blocks) {
                input_from_buffer_length += blk->size;
            }
            move_bof(view);
        }
        break;
    case EXEC_WORD:
        input_from_buffer = true;
        if (!view->selection) {
            CurrentLineRef lr = get_current_line_and_offset(view->cursor);
            WordBounds wb = get_bounds_for_word_under_cursor(lr);
            if (wb.end == 0) {
                break;
            }

            // If `wb.start` is less than `lr.cursor_offset` here, the
            // subtraction wraps but nevertheless works as intended.
            // view->cursor.offset -= (lr.cursor_offset - wb.start) ==
            view->cursor.offset += wb.start - lr.cursor_offset;

            input_from_buffer_length = wb.end - wb.start;
            BUG_ON(view->cursor.offset >= view->cursor.blk->size);
        }
        break;
    case EXEC_MSG:
    case EXEC_MSG_A:
    case EXEC_MSG_B:
    case EXEC_MSG_C:
        input = dump_messages(&e->messages[msgs_idx_from_exec_action(in_action)]);
        break;
    case EXEC_COMMAND:
        input = history_dump(&e->command_history);
        break;
    case EXEC_SEARCH:
        input = history_dump(&e->search_history);
        break;
    case EXEC_OPEN:
        input = file_history_dump(&e->file_history);
        break;
    case EXEC_OPEN_REL:
        input = file_history_dump_relative(&e->file_history);
        break;
    case EXEC_NULL:
    case EXEC_TTY:
        break;
    // These can't be used as input actions and should be prevented by
    // the validity checks in cmd_exec():
    case EXEC_TAG:
    case EXEC_TAG_A:
    case EXEC_TAG_B:
    case EXEC_TAG_C:
    case EXEC_ECHO:
    case EXEC_EVAL:
    case EXEC_ERRMSG:
    case EXEC_INVALID:
    default:
        BUG("unhandled action");
        return -1;
    }

    // This could be left uninitialized, but doing so makes some old compilers
    // produce false-positive "-Wmaybe-uninitialized" warnings
    SelectionInfo info = {.si = view->cursor};

    if (view->selection && (input_from_buffer || output_to_buffer)) {
        info = init_selection(view);
        view->cursor = info.si;
        if (input_from_buffer) {
            input_from_buffer_length = info.eo - info.so;
        }
    }

    if (input_from_buffer) {
        input = block_iter_get_bytes(view->cursor, input_from_buffer_length);
    } else {
        BUG_ON(input_from_buffer_length);
    }

    ctx.input = strview_from_string(&input);
    yield_terminal(e, quiet);
    int err = spawn(&ctx);
    bool prompt = (err >= 0) && (exec_flags & EXECFLAG_PROMPT);
    resume_terminal(e, quiet, prompt);
    string_free(&input);
    ctx.input.length = 0;

    if (err != 0) {
        show_spawn_error_msg(&e->err, strview_from_string(&ctx.outputs[1]), err);
        string_free(&ctx.outputs[0]);
        string_free(&ctx.outputs[1]);
        view->cursor = saved_cursor;
        return -1;
    }

    string_free(&ctx.outputs[1]);
    String *output = &ctx.outputs[0];
    bool strip_trailing_newline = (exec_flags & EXECFLAG_STRIP_NL);
    if (
        strip_trailing_newline
        && output_to_buffer
        && output->len > 0
        && output->buffer[output->len - 1] == '\n'
    ) {
        output->len--;
        if (output->len > 0 && output->buffer[output->len - 1] == '\r') {
            output->len--;
        }
    }

    if (!output_to_buffer) {
        view->cursor = saved_cursor;
        view->sel_so = saved_sel_so;
        view->sel_eo = saved_sel_eo;
        mark_all_lines_changed(view->buffer);
    }

    ExecAction out_action = actions[STDOUT_FILENO];
    switch (out_action) {
    case EXEC_BUFFER:
        if (view->selection) {
            insert_to_selection(view, strview_from_string(output), &info);
        } else {
            // Replace any unselected text used as input, e.g. for `-i line`
            size_t del_count = input_from_buffer_length;
            buffer_replace_bytes(view, del_count, strview_from_string(output));
        }
        break;
    case EXEC_ECHO:
        if (output->len) {
            size_t pos = 0;
            StringView line = buf_slice_next_line(output->buffer, &pos, output->len);
            info_msg(&e->err, "%.*s", SV_FMT(line));
        }
        break;
    case EXEC_MSG:
    case EXEC_MSG_A:
    case EXEC_MSG_B:
    case EXEC_MSG_C:
        parse_and_activate_message(e, strview_from_string(output), out_action);
        break;
    case EXEC_OPEN:
        open_files_from_string(e, output);
        break;
    case EXEC_TAG:
    case EXEC_TAG_A:
    case EXEC_TAG_B:
    case EXEC_TAG_C:
        parse_and_activate_tags(e, strview_from_string(output), out_action);
        break;
    case EXEC_EVAL:
        exec_normal_config(e, strview_from_string(output));
        break;
    case EXEC_NULL:
    case EXEC_TTY:
        break;
    // These can't be used as output actions
    case EXEC_COMMAND:
    case EXEC_ERRMSG:
    case EXEC_LINE:
    case EXEC_OPEN_REL:
    case EXEC_SEARCH:
    case EXEC_WORD:
    case EXEC_INVALID:
    default:
        BUG("unhandled action");
        return -1;
    }

    size_t output_len = output->len;
    string_free(output);
    return output_len;
}

void yield_terminal(EditorState *e, bool quiet)
{
    if (e->flags & EFLAG_HEADLESS) {
        return;
    }

    if (quiet) {
        term_raw_isig();
    } else {
        need_term_reset_on_fatal_error = 0;
        ui_end(&e->terminal, false);
        term_cooked();
    }
}

void resume_terminal(EditorState *e, bool quiet, bool prompt)
{
    if (e->flags & EFLAG_HEADLESS) {
        return;
    }

    term_raw();
    if (!quiet) {
        BUG_ON(need_term_reset_on_fatal_error);
        if (prompt) {
            any_key(&e->terminal, e->options.esc_timeout);
        }
        ui_start(e);
        need_term_reset_on_fatal_error = 1;
    }
}
