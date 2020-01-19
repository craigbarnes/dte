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
    const char *str = string_borrow_cstring(&editor.cmdline.buf);
    cmdline_clear(&editor.cmdline);
    if (str[0] != ' ') {
        // This is done before handle_command() because "command [text]"
        // can modify the contents of the command-line
        history_add(&editor.command_history, str, command_history_size);
    }
    handle_command(&commands, str);
}

static void command_mode_keypress(KeyCode key)
{
    switch (key) {
    case KEY_ENTER:
        command_mode_handle_enter();
        return;
    case '\t':
        complete_command_next();
        return;
    case MOD_SHIFT | '\t':
        complete_command_prev();
        return;
    }

    switch (cmdline_handle_key(&editor.cmdline, &editor.command_history, key)) {
    case CMDLINE_CANCEL:
        set_input_mode(INPUT_NORMAL);
        // Fallthrough
    case CMDLINE_KEY_HANDLED:
        reset_completion();
        // Fallthrough
    case CMDLINE_UNKNOWN_KEY:
        return;
    }
}

const EditorModeOps command_mode_ops = {
    .keypress = command_mode_keypress,
    .update = normal_update,
};
