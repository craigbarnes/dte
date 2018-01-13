#include "cmdline.h"
#include "history.h"
#include "editor.h"
#include "command.h"
#include "completion.h"
#include "error.h"

static void command_line_enter(void)
{
    PointerArray array = PTR_ARRAY_INIT;
    char *str = string_cstring(&editor.cmdline.buf);
    Error *err = NULL;

    reset_completion();
    set_input_mode(INPUT_NORMAL);
    bool ok = parse_commands(&array, str, &err);

    // Need to do this before executing the command because
    // "command" can modify contents of command line.
    history_add(&editor.command_history, str, command_history_size);
    free(str);
    cmdline_clear(&editor.cmdline);

    if (ok) {
        run_commands(commands, &array);
    } else {
        error_msg("Parsing command: %s", err->msg);
        error_free(err);
    }
    ptr_array_free(&array);
}

static void command_mode_keypress(Key key)
{
    switch (key) {
    case KEY_ENTER:
        command_line_enter();
        break;
    case '\t':
        complete_command();
        break;
    default:
        switch (cmdline_handle_key(&editor.cmdline, &editor.command_history, key)) {
        case CMDLINE_UNKNOWN_KEY:
            break;
        case CMDLINE_KEY_HANDLED:
            reset_completion();
            break;
        case CMDLINE_CANCEL:
            set_input_mode(INPUT_NORMAL);
            break;
        }
    }
}

const EditorModeOps command_mode_ops = {
    .keypress = command_mode_keypress,
    .update = normal_update,
};
