#include "macro.h"
#include "serialize.h"
#include "util/string-view.h"

static void merge_insert_buffer(MacroRecorder *m)
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

bool macro_record(MacroRecorder *m)
{
    if (m->recording) {
        return false;
    }
    ptr_array_free(&m->prev_macro);
    m->prev_macro = m->macro;
    m->macro = (PointerArray) PTR_ARRAY_INIT;
    m->recording = true;
    return true;
}

bool macro_stop(MacroRecorder *m)
{
    if (!m->recording) {
        return false;
    }
    merge_insert_buffer(m);
    m->recording = false;
    return true;
}

bool macro_cancel(MacroRecorder *m)
{
    if (!m->recording) {
        return false;
    }
    ptr_array_free(&m->macro);
    m->macro = m->prev_macro;
    m->prev_macro = (PointerArray) PTR_ARRAY_INIT;
    m->recording = false;
    return true;
}

void macro_command_hook(MacroRecorder *m, const char *cmd_name, char **args)
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

void macro_search_hook (
    MacroRecorder *m,
    const char *pattern,
    bool reverse,
    bool add_to_history
) {
    if (!m->recording) {
        return;
    }

    char *cmd;
    if (pattern) {
        StringView pat = strview(pattern);
        String buf = string_new(pat.length + sizeof("search -r -H -- ") + 8);
        string_append_cstring(&buf, "search ");
        string_append_cstring(&buf, reverse ? "-r " : "");
        string_append_cstring(&buf, add_to_history ? "" : "-H ");
        string_append_cstring(&buf, unlikely(pattern[0] == '-') ? "-- " : "");
        string_append_escaped_arg_sv(&buf, pat, true);
        cmd = string_steal_cstring(&buf);
    } else {
        cmd = xstrdup(reverse ? "search -p" : "search -n");
    }

    merge_insert_buffer(m);
    ptr_array_append(&m->macro, cmd);
}

void macro_insert_char_hook(MacroRecorder *m, CodePoint c)
{
    if (m->recording) {
        string_append_codepoint(&m->insert_buffer, c);
    }
}

void macro_insert_text_hook(MacroRecorder *m, const char *text, size_t size)
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

String dump_macro(const MacroRecorder *m)
{
    String buf = string_new(4096);
    for (size_t i = 0, n = m->macro.count; i < n; i++) {
        const char *cmd_str = m->macro.ptrs[i];
        string_append_cstring(&buf, cmd_str);
        string_append_byte(&buf, '\n');
    }
    return buf;
}

void free_macro(MacroRecorder *m)
{
    string_free(&m->insert_buffer);
    ptr_array_free(&m->macro);
    ptr_array_free(&m->prev_macro);
    m->recording = false;
}
