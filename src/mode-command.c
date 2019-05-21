#include "cmdline.h"
#include "command.h"
#include "completion.h"
#include "editor.h"
#include "error.h"
#include "history.h"
#include "mode.h"

static void command_mode_handle_enter(void)
{
    reset_completion();
    set_input_mode(INPUT_NORMAL);

    string_ensure_null_terminated(&editor.cmdline.buf);
    const char *str = editor.cmdline.buf.buffer;

    PointerArray array = PTR_ARRAY_INIT;
    Error *err = NULL;
    bool ok = parse_commands(&array, str, &err);

    // This is done before run_commands() because "command [text]"
    // can modify the contents of the command-line
    history_add(&editor.command_history, str, command_history_size);
    cmdline_clear(&editor.cmdline);

    if (ok) {
        run_commands(commands, &array);
    } else {
        error_msg("Parsing command: %s", err->msg);
        error_free(err);
    }

    ptr_array_free(&array);
}

static void command_mode_keypress(KeyCode key)
{
    switch (key) {
    case KEY_ENTER:
        command_mode_handle_enter();
        return;
    case '\t':
        complete_command();
        return;
    }

    switch (cmdline_handle_key(&editor.cmdline, &editor.command_history, key)) {
    case CMDLINE_KEY_HANDLED:
        reset_completion();
        return;
    case CMDLINE_CANCEL:
        set_input_mode(INPUT_NORMAL);
        return;
    case CMDLINE_UNKNOWN_KEY:
        return;
    }
}

const EditorModeOps command_mode_ops = {
    .keypress = command_mode_keypress,
    .update = normal_update,
};
