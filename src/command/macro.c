#include "macro.h"
#include "commands.h"
#include "error.h"
#include "run.h"
#include "serialize.h"
#include "util/ptr-array.h"
#include "util/string-view.h"

static PointerArray macro = PTR_ARRAY_INIT;
static PointerArray prev_macro = PTR_ARRAY_INIT;
static String insert_buffer = STRING_INIT;
static bool recording_macro = false;

static void merge_insert_buffer(void)
{
    size_t len = insert_buffer.len;
    if (len == 0) {
        return;
    }
    String s = string_new(32 + len);
    StringView ibuf = strview_from_string(&insert_buffer);
    string_append_literal(&s, "insert -km ");
    if (unlikely(strview_has_prefix(&ibuf, "-"))) {
        string_append_literal(&s, "-- ");
    }
    string_append_escaped_arg_sv(&s, ibuf, true);
    string_clear(&insert_buffer);
    ptr_array_append(&macro, string_steal_cstring(&s));
}

bool macro_is_recording(void)
{
    return recording_macro;
}

void macro_record(void)
{
    if (recording_macro) {
        info_msg("Already recording");
        return;
    }
    ptr_array_free(&prev_macro);
    prev_macro = macro;
    macro = (PointerArray) PTR_ARRAY_INIT;
    info_msg("Recording macro");
    recording_macro = true;
}

void macro_stop(void)
{
    if (!recording_macro) {
        info_msg("Not recording");
        return;
    }
    merge_insert_buffer();
    const size_t count = macro.count;
    const char *plural = (count > 1) ? "s" : "";
    info_msg("Macro recording stopped; %zu command%s saved", count, plural);
    recording_macro = false;
}

void macro_toggle(void)
{
    if (recording_macro) {
        macro_stop();
    } else {
        macro_record();
    }
}

void macro_cancel(void)
{
    if (!recording_macro) {
        info_msg("Not recording");
        return;
    }
    ptr_array_free(&macro);
    macro = prev_macro;
    prev_macro = (PointerArray) PTR_ARRAY_INIT;
    info_msg("Macro recording cancelled");
    recording_macro = false;
}

void macro_command_hook(const char *cmd_name, char **args)
{
    if (!recording_macro) {
        return;
    }
    String buf = string_new(512);
    string_append_cstring(&buf, cmd_name);
    for (size_t i = 0; args[i]; i++) {
        string_append_byte(&buf, ' ');
        string_append_escaped_arg(&buf, args[i], true);
    }
    merge_insert_buffer();
    ptr_array_append(&macro, string_steal_cstring(&buf));
}

void macro_insert_char_hook(CodePoint c)
{
    if (!recording_macro) {
        return;
    }
    string_append_codepoint(&insert_buffer, c);
}

void macro_insert_text_hook(const char *text, size_t size)
{
    if (!recording_macro) {
        return;
    }
    String buf = string_new(512);
    StringView sv = string_view(text, size);
    string_append_literal(&buf, "insert -m ");
    if (unlikely(strview_has_prefix(&sv, "-"))) {
        string_append_literal(&buf, "-- ");
    }
    string_append_escaped_arg_sv(&buf, sv, true);
    merge_insert_buffer();
    ptr_array_append(&macro, string_steal_cstring(&buf));
}

void macro_play(void)
{
    unsigned int saved_nr_errors = get_nr_errors();
    for (size_t i = 0, n = macro.count; i < n; i++) {
        const char *cmd_str = macro.ptrs[i];
        handle_command(&commands, cmd_str, false);
        if (get_nr_errors() != saved_nr_errors) {
            break;
        }
    }
}

String dump_macro(void)
{
    String buf = string_new(4096);
    for (size_t i = 0, n = macro.count; i < n; i++) {
        const char *cmd_str = macro.ptrs[i];
        string_append_cstring(&buf, cmd_str);
        string_append_byte(&buf, '\n');
    }
    return buf;
}
