#include "macro.h"
#include "commands.h"
#include "error.h"
#include "run.h"
#include "serialize.h"
#include "util/string-view.h"

static void merge_insert_buffer(CommandMacroState *m)
{
    size_t len = m->insert_buffer.len;
    if (len == 0) {
        return;
    }
    String s = string_new(32 + len);
    StringView ibuf = strview_from_string(&m->insert_buffer);
    string_append_literal(&s, "insert -k ");
    if (unlikely(strview_has_prefix(&ibuf, "-"))) {
        string_append_literal(&s, "-- ");
    }
    string_append_escaped_arg_sv(&s, ibuf, true);
    string_clear(&m->insert_buffer);
    ptr_array_append(&m->macro, string_steal_cstring(&s));
}

void macro_record(CommandMacroState *m)
{
    if (m->recording) {
        info_msg("Already recording");
        return;
    }
    ptr_array_free(&m->prev_macro);
    m->prev_macro = m->macro;
    m->macro = (PointerArray) PTR_ARRAY_INIT;
    info_msg("Recording macro");
    m->recording = true;
}

void macro_stop(CommandMacroState *m)
{
    if (!m->recording) {
        info_msg("Not recording");
        return;
    }
    merge_insert_buffer(m);
    size_t count = m->macro.count;
    const char *plural = (count != 1) ? "s" : "";
    info_msg("Macro recording stopped; %zu command%s saved", count, plural);
    m->recording = false;
}

void macro_toggle(CommandMacroState *m)
{
    if (m->recording) {
        macro_stop(m);
    } else {
        macro_record(m);
    }
}

void macro_cancel(CommandMacroState *m)
{
    if (!m->recording) {
        info_msg("Not recording");
        return;
    }
    ptr_array_free(&m->macro);
    m->macro = m->prev_macro;
    m->prev_macro = (PointerArray) PTR_ARRAY_INIT;
    info_msg("Macro recording cancelled");
    m->recording = false;
}

void macro_command_hook(CommandMacroState *m, const char *cmd_name, char **args)
{
    if (!m->recording) {
        return;
    }
    String buf = string_new(512);
    string_append_cstring(&buf, cmd_name);
    for (size_t i = 0; args[i]; i++) {
        string_append_byte(&buf, ' ');
        string_append_escaped_arg(&buf, args[i], true);
    }
    merge_insert_buffer(m);
    ptr_array_append(&m->macro, string_steal_cstring(&buf));
}

void macro_insert_char_hook(CommandMacroState *m, CodePoint c)
{
    if (!m->recording) {
        return;
    }
    string_append_codepoint(&m->insert_buffer, c);
}

void macro_insert_text_hook(CommandMacroState *m, const char *text, size_t size)
{
    if (!m->recording) {
        return;
    }
    String buf = string_new(512);
    StringView sv = string_view(text, size);
    string_append_literal(&buf, "insert -m ");
    if (unlikely(strview_has_prefix(&sv, "-"))) {
        string_append_literal(&buf, "-- ");
    }
    string_append_escaped_arg_sv(&buf, sv, true);
    merge_insert_buffer(m);
    ptr_array_append(&m->macro, string_steal_cstring(&buf));
}

void macro_play(CommandMacroState *m)
{
    unsigned int saved_nr_errors = get_nr_errors();
    for (size_t i = 0, n = m->macro.count; i < n; i++) {
        const char *cmd_str = m->macro.ptrs[i];
        handle_command(&normal_commands, cmd_str, false);
        if (get_nr_errors() != saved_nr_errors) {
            break;
        }
    }
}

String dump_macro(const CommandMacroState *m)
{
    String buf = string_new(4096);
    for (size_t i = 0, n = m->macro.count; i < n; i++) {
        const char *cmd_str = m->macro.ptrs[i];
        string_append_cstring(&buf, cmd_str);
        string_append_byte(&buf, '\n');
    }
    return buf;
}

void free_macro(CommandMacroState *m)
{
    string_free(&m->insert_buffer);
    ptr_array_free(&m->macro);
    ptr_array_free(&m->prev_macro);
    m->recording = false;
}
