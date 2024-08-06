#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "exec.h"
#include "block-iter.h"
#include "buffer.h"
#include "change.h"
#include "command/macro.h"
#include "commands.h"
#include "ctags.h"
#include "error.h"
#include "misc.h"
#include "move.h"
#include "msg.h"
#include "selection.h"
#include "tag.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/string-view.h"
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
    [EXEC_ERRMSG] = {"errmsg", ERR},
    [EXEC_EVAL] = {"eval", OUT},
    [EXEC_LINE] = {"line", IN},
    [EXEC_MSG] = {"msg", IN | OUT},
    [EXEC_NULL] = {"null", ALL},
    [EXEC_OPEN] = {"open", IN | OUT},
    [EXEC_OPEN_REL] = {"open-rel", IN},
    [EXEC_SEARCH] = {"search", IN},
    [EXEC_TAG] = {"tag", OUT},
    [EXEC_TTY] = {"tty", ALL},
    [EXEC_WORD] = {"word", IN},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(exec_map, name, strcmp);
}

ExecAction lookup_exec_action(const char *name, int fd)
{
    BUG_ON(fd < 0 || fd > 2);
    ssize_t i = BSEARCH_IDX(name, exec_map, vstrcmp);
    return (i >= 0 && (exec_map[i].flags & 1u << fd)) ? i : EXEC_INVALID;
}

void collect_exec_actions(PointerArray *a, const char *prefix, int fd)
{
    if (unlikely(fd < 0 || fd > 2)) {
        return;
    }

    unsigned int flag = 1u << fd;
    for (size_t i = 0; i < ARRAYLEN(exec_map); i++) {
        const char *action = exec_map[i].name;
        if ((exec_map[i].flags & flag) && str_has_prefix(action, prefix)) {
            ptr_array_append(a, xstrdup(action));
        }
    }
}

static void open_files_from_string(EditorState *e, const String *str)
{
    PointerArray filenames = PTR_ARRAY_INIT;
    for (size_t pos = 0, size = str->len; pos < size; ) {
        char *filename = buf_next_line(str->buffer, &pos, size);
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

static void parse_and_activate_message(EditorState *e, const String *str)
{
    if (unlikely(str->len == 0)) {
        error_msg("child produced no output");
        return;
    }

    MessageArray *msgs = &e->messages;
    size_t count = msgs->array.count;
    size_t x;
    if (!count || !buf_parse_size(str->buffer, str->len, &x) || !x) {
        return;
    }

    msgs->pos = MIN(x - 1, count - 1);
    activate_current_message(msgs, e->window);
}

static void parse_and_goto_tag(EditorState *e, const String *str)
{
    if (unlikely(str->len == 0)) {
        error_msg("child produced no output");
        return;
    }

    size_t pos = 0;
    StringView line = buf_slice_next_line(str->buffer, &pos, str->len);
    if (line.length == 0) {
        return;
    }

    MessageArray *msgs = &e->messages;
    Tag tag;
    bool parsed = parse_ctags_line(&tag, line.data, line.length);

    if (parsed) {
        char cwd[8192];
        if (unlikely(!getcwd(cwd, sizeof cwd))) {
            error_msg_errno("getcwd() failed");
            return;
        }
        StringView dir = strview_from_cstring(cwd);
        clear_messages(msgs);
        add_message_for_tag(msgs, &tag, &dir);
    } else {
        // Treat line as a simple tag name
        if (!tag_lookup(&e->tagfile, &line, e->buffer->abs_filename, msgs)) {
            return;
        }
    }

    activate_current_message_save(msgs, &e->bookmarks, e->view);
}

static void insert_output (
    View *view,
    const String *output,
    const StringView *input,
    SelectionInfo *info,
    bool replace_input
) {
    if (!view->selection) {
        size_t del_count = replace_input ? input->length : 0;
        buffer_replace_bytes(view, del_count, output->buffer, output->len);
        return;
    }

    if (!replace_input) {
        // There's a selection but it wasn't used for stdin in this case,
        // so SelectionInfo needs to be initialized here
        *info = init_selection(view);
        view->cursor = info->si;
    }

    size_t del_count = info->eo - info->so;
    buffer_replace_bytes(view, del_count, output->buffer, output->len);

    if (output->len == 0) {
        // If the selection was replaced with 0 bytes then there's nothing
        // new to select, so just unselect instead
        unselect(view);
        return;
    }

    // Keep the selection and adjust the size to the newly inserted text
    size_t so = info->so;
    size_t eo = so + (output->len - 1);
    block_iter_goto_offset(&view->cursor, info->swapped ? so : eo);
    view->sel_so = info->swapped ? eo : so;
    view->sel_eo = SEL_EO_RECALC;
}

static const char **lines_and_columns_env(const Window *window)
{
    static char lines[DECIMAL_STR_MAX(window->edit_h)];
    static char columns[DECIMAL_STR_MAX(window->edit_w)];
    static const char *vars[] = {
        "LINES", lines,
        "COLUMNS", columns,
        NULL,
    };

    buf_uint_to_str(window->edit_h, lines);
    buf_uint_to_str(window->edit_w, columns);
    return vars;
}

static void show_spawn_error_msg(const String *errstr, int err)
{
    if (err <= 0) {
        return;
    }

    char msg[512];
    msg[0] = '\0';
    if (errstr->len) {
        size_t pos = 0;
        StringView line = buf_slice_next_line(errstr->buffer, &pos, errstr->len);
        BUG_ON(pos == 0);
        size_t len = MIN(line.length, sizeof(msg) - 8);
        xsnprintf(msg, sizeof(msg), ": \"%.*s\"", (int)len, line.data);
    }

    if (err >= 256) {
        int sig = err >> 8;
        const char *str = strsignal(sig);
        error_msg("Child received signal %d (%s)%s", sig, str ? str : "??", msg);
    } else if (err) {
        error_msg("Child returned %d%s", err, msg);
    }
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

ssize_t handle_exec (
    EditorState *e,
    const char **argv,
    ExecAction actions[3],
    SpawnFlags spawn_flags,
    bool strip_trailing_newline
) {
    View *view = e->view;
    const BlockIter saved_cursor = view->cursor;
    const ssize_t saved_sel_so = view->sel_so;
    const ssize_t saved_sel_eo = view->sel_eo;
    char *alloc = NULL;
    bool output_to_buffer = (actions[STDOUT_FILENO] == EXEC_BUFFER);
    bool replace_input = false;
    SelectionInfo info;

    SpawnContext ctx = {
        .editor = e,
        .argv = argv,
        .outputs = {STRING_INIT, STRING_INIT},
        .flags = spawn_flags,
        .env = output_to_buffer ? lines_and_columns_env(e->window) : NULL,
        .actions = {
            spawn_action_from_exec_action(actions[0]),
            spawn_action_from_exec_action(actions[1]),
            spawn_action_from_exec_action(actions[2]),
        },
    };

    switch (actions[STDIN_FILENO]) {
    case EXEC_LINE:
        if (view->selection) {
            info = init_selection(view);
            view->cursor = info.si;
            ctx.input.length = info.eo - info.so;
        } else {
            move_bol(view);
            StringView line = block_iter_get_line(&view->cursor);
            ctx.input.length = line.length;
        }
        replace_input = true;
        get_bytes:
        alloc = block_iter_get_bytes(&view->cursor, ctx.input.length);
        ctx.input.data = alloc;
        break;
    case EXEC_BUFFER:
        if (view->selection) {
            info = init_selection(view);
            view->cursor = info.si;
            ctx.input.length = info.eo - info.so;
        } else {
            const Block *blk;
            block_for_each(blk, &view->buffer->blocks) {
                ctx.input.length += blk->size;
            }
            move_bof(view);
        }
        replace_input = true;
        goto get_bytes;
    case EXEC_WORD:
        if (view->selection) {
            info = init_selection(view);
            view->cursor = info.si;
            ctx.input.length = info.eo - info.so;
            replace_input = true;
        } else {
            StringView line;
            size_t offset = fetch_this_line(&view->cursor, &line);
            StringView word = get_word_under_cursor(line, offset);
            if (word.length == 0) {
                break;
            }
            // TODO: optimize this, so that the BlockIter moves by just the
            // minimal word offset instead of iterating to a line offset
            ctx.input.length = word.length;
            move_bol(view);
            view->cursor.offset += offset;
            BUG_ON(view->cursor.offset >= view->cursor.blk->size);
        }
        goto get_bytes;
    case EXEC_MSG: {
        String messages = dump_messages(&e->messages);
        ctx.input = strview_from_string(&messages);
        alloc = messages.buffer;
        break;
    }
    case EXEC_COMMAND: {
        String hist = history_dump(&e->command_history);
        ctx.input = strview_from_string(&hist);
        alloc = hist.buffer;
        break;
    }
    case EXEC_SEARCH: {
        String hist = history_dump(&e->search_history);
        ctx.input = strview_from_string(&hist);
        alloc = hist.buffer;
        break;
    }
    case EXEC_OPEN: {
        String hist = file_history_dump(&e->file_history);
        ctx.input = strview_from_string(&hist);
        alloc = hist.buffer;
        break;
    }
    case EXEC_OPEN_REL: {
        String hist = file_history_dump_relative(&e->file_history);
        ctx.input = strview_from_string(&hist);
        alloc = hist.buffer;
        break;
    }
    case EXEC_NULL:
    case EXEC_TTY:
        break;
    // These can't be used as input actions and should be prevented by
    // the validity checks in cmd_exec():
    case EXEC_TAG:
    case EXEC_EVAL:
    case EXEC_ERRMSG:
    case EXEC_INVALID:
    default:
        BUG("unhandled action");
        return -1;
    }

    int err = spawn(&ctx);
    free(alloc);
    if (err != 0) {
        show_spawn_error_msg(&ctx.outputs[1], err);
        string_free(&ctx.outputs[0]);
        string_free(&ctx.outputs[1]);
        view->cursor = saved_cursor;
        return -1;
    }

    string_free(&ctx.outputs[1]);
    String *output = &ctx.outputs[0];
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

    switch (actions[STDOUT_FILENO]) {
    case EXEC_BUFFER:
        insert_output(view, output, &ctx.input, &info, replace_input);
        break;
    case EXEC_MSG:
        parse_and_activate_message(e, output);
        break;
    case EXEC_OPEN:
        open_files_from_string(e, output);
        break;
    case EXEC_TAG:
        parse_and_goto_tag(e, output);
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
