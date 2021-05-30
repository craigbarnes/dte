#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command/args.h"
#include "command/macro.h"
#include "command/parse.h"
#include "command/serialize.h"
#include "commands.h"
#include "completion.h"
#include "error.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

typedef struct {
    KeyCode key;
    KeyBinding *bind;
} KeyBindingEntry;

typedef struct {
    const CommandSet *const cmds;

    // Fast lookup table for most common key combinations (Ctrl or Meta
    // with ASCII keys or any combination of modifiers with special keys)
    KeyBinding *table[(2 * 128) + (8 * NR_SPECIAL_KEYS)];

    // Fallback for all other keys (Unicode combos etc.)
    PointerArray ptr_array;
} KeyBindingGroup;

static KeyBindingGroup bindings[] = {
    [INPUT_NORMAL] = {.cmds = &normal_commands},
    [INPUT_COMMAND] = {.cmds = &cmd_mode_commands},
    [INPUT_SEARCH] = {.cmds = &cmd_mode_commands},
};

static ssize_t get_lookup_table_index(KeyCode k)
{
    const KeyCode modifiers = keycode_get_modifiers(k);
    const KeyCode key = keycode_get_key(k);

    static_assert(MOD_MASK >> MOD_OFFSET == (1 | 2 | 4));

    if (key >= KEY_SPECIAL_MIN && key <= KEY_SPECIAL_MAX) {
        const size_t mod_offset = (modifiers >> MOD_OFFSET) * NR_SPECIAL_KEYS;
        return (2 * 128) + mod_offset + (key - KEY_SPECIAL_MIN);
    }

    if (key >= 0x20 && key <= 0x7E) {
        switch (modifiers) {
        case MOD_CTRL: return key;
        case MOD_META: return key + 128;
        }
    }

    return -1;
}

UNITTEST {
    const size_t size = ARRAY_COUNT(bindings[0].table);
    const KeyCode min = KEY_SPECIAL_MIN;
    const KeyCode max = KEY_SPECIAL_MAX;
    const KeyCode nsk = NR_SPECIAL_KEYS;
    BUG_ON(size != 256 + (8 * NR_SPECIAL_KEYS));
    BUG_ON(get_lookup_table_index(MOD_MASK | max) != size - 1);
    BUG_ON(get_lookup_table_index(min) != 256);
    BUG_ON(get_lookup_table_index(max) != 256 + nsk - 1);
    BUG_ON(get_lookup_table_index(MOD_SHIFT | min) != 256 + nsk);
    BUG_ON(get_lookup_table_index(MOD_CTRL | max) != 256 + (5 * nsk) - 1);

    BUG_ON(get_lookup_table_index(MOD_CTRL | KEY_SPACE) != 32);
    BUG_ON(get_lookup_table_index(MOD_META | KEY_SPACE) != 32 + 128);
    BUG_ON(get_lookup_table_index(MOD_CTRL | '~') != 126);
    BUG_ON(get_lookup_table_index(MOD_META | '~') != 126 + 128);

    BUG_ON(get_lookup_table_index(MOD_CTRL | MOD_META | 'a') != -1);
    BUG_ON(get_lookup_table_index(MOD_META | 0x0E01) != -1);
}

static KeyBinding *key_binding_new(InputMode mode, const char *cmd_str)
{
    const size_t cmd_str_len = strlen(cmd_str);
    KeyBinding *b = xmalloc(sizeof(*b) + cmd_str_len + 1);
    b->cmd = NULL;
    memcpy(b->cmd_str, cmd_str, cmd_str_len + 1);

    PointerArray array = PTR_ARRAY_INIT;
    if (parse_commands(&array, cmd_str) != CMDERR_NONE) {
        goto out;
    }

    ptr_array_trim_nulls(&array);
    if (array.count < 2 || ptr_array_idx(&array, NULL) != (array.count - 1)) {
        // Only single commands can be cached
        goto out;
    }

    const Command *cmd = bindings[mode].cmds->lookup(array.ptrs[0]);
    if (!cmd) {
        // Aliases or non-existent commands can't be cached
        goto out;
    }

    if (memchr(cmd_str, '$', cmd_str_len)) {
        // Commands containing variables can't be cached
        goto out;
    }

    free(ptr_array_remove_idx(&array, 0));
    CommandArgs a = {.args = (char**)array.ptrs};
    if (do_parse_args(cmd, &a) != 0) {
        goto out;
    }

    // Command can be cached; binding takes ownership of args array
    b->cmd = cmd;
    b->a = a;
    return b;

out:
    ptr_array_free(&array);
    return b;
}

static void key_binding_free(KeyBinding *binding)
{
    if (binding) {
        if (binding->cmd) {
            free_string_array(binding->a.args);
        }
        free(binding);
    }
}

void add_binding(InputMode mode, const char *keystr, const char *command)
{
    KeyCode key;
    if (!parse_key_string(&key, keystr)) {
        error_msg("invalid key string: %s", keystr);
        return;
    }

    const ssize_t idx = get_lookup_table_index(key);
    if (idx >= 0) {
        KeyBinding **table = bindings[mode].table;
        key_binding_free(table[idx]);
        table[idx] = key_binding_new(mode, command);
        return;
    }

    KeyBindingEntry *b = xnew(KeyBindingEntry, 1);
    b->key = key;
    b->bind = key_binding_new(mode, command);
    ptr_array_append(&bindings[mode].ptr_array, b);
}

void remove_binding(InputMode mode, const char *keystr)
{
    KeyCode key;
    if (!parse_key_string(&key, keystr)) {
        return;
    }

    const ssize_t idx = get_lookup_table_index(key);
    if (idx >= 0) {
        KeyBinding **table = bindings[mode].table;
        key_binding_free(table[idx]);
        table[idx] = NULL;
        return;
    }

    PointerArray *ptr_array = &bindings[mode].ptr_array;
    size_t i = ptr_array->count;
    while (i > 0) {
        KeyBindingEntry *b = ptr_array->ptrs[--i];
        if (b->key == key) {
            ptr_array_remove_idx(ptr_array, i);
            key_binding_free(b->bind);
            free(b);
            return;
        }
    }
}

const KeyBinding *lookup_binding(InputMode mode, KeyCode key)
{
    const ssize_t idx = get_lookup_table_index(key);
    if (idx >= 0) {
        return bindings[mode].table[idx];
    }

    PointerArray *ptr_array = &bindings[mode].ptr_array;
    for (size_t i = ptr_array->count; i > 0; i--) {
        KeyBindingEntry *b = ptr_array->ptrs[i - 1];
        if (b->key == key) {
            return b->bind;
        }
    }

    return NULL;
}

bool handle_binding(InputMode mode, KeyCode key)
{
    const KeyBinding *b = lookup_binding(mode, key);
    if (!b) {
        return false;
    }

    // If the command isn't cached or a macro is being recorded
    if (!b->cmd || macro_is_recording()) {
        // Parse and run command string
        const CommandSet *cmds = bindings[mode].cmds;
        handle_command(cmds, b->cmd_str, !!cmds->allow_recording);
        return true;
    }

    // Command is cached; call it directly
    begin_change(CHANGE_MERGE_NONE);
    current_command = b->cmd;
    b->cmd->cmd(&b->a);
    current_command = NULL;
    end_change();
    return true;
}

static void maybe_add_key_completion(const char *prefix, KeyCode k)
{
    const char *str = keycode_to_string(k);
    if (str_has_prefix(str, prefix)) {
        add_completion(xstrdup(str));
    }
}

static void maybe_add_lt_key_completion(const char *prefix, KeyCode k)
{
    const ssize_t idx = get_lookup_table_index(k);
    BUG_ON(idx < 0);
    if (bindings[INPUT_NORMAL].table[idx]) {
        maybe_add_key_completion(prefix, k);
    }
}

void collect_bound_keys(const char *prefix)
{
    for (KeyCode k = 0x20; k < 0x7E; k++) {
        maybe_add_lt_key_completion(prefix, MOD_CTRL | k);
    }

    for (KeyCode k = 0x20; k < 0x7E; k++) {
        maybe_add_lt_key_completion(prefix, MOD_META | k);
    }

    static_assert(MOD_MASK >> MOD_OFFSET == 7);
    for (KeyCode m = 0, mods = 0; m <= 7; mods = ++m << MOD_OFFSET) {
        for (KeyCode k = KEY_SPECIAL_MIN; k <= KEY_SPECIAL_MAX; k++) {
            maybe_add_lt_key_completion(prefix, mods | k);
        }
    }

    const PointerArray *array = &bindings[INPUT_NORMAL].ptr_array;
    for (size_t i = 0, n = array->count; i < n; i++) {
        const KeyBindingEntry *b = array->ptrs[i];
        maybe_add_key_completion(prefix, b->key);
    }
}

static void append_binding(String *s, KeyCode key, const char *flag, const char *cmd)
{
    string_append_literal(s, "bind ");
    string_append_cstring(s, flag);
    string_append_escaped_arg(s, keycode_to_string(key), true);
    string_append_byte(s, ' ');
    string_append_escaped_arg(s, cmd, true);
    string_append_byte(s, '\n');
}

static void append_lookup_table_binding(String *s, InputMode mode, const char *flag, KeyCode key)
{
    const ssize_t i = get_lookup_table_index(key);
    BUG_ON(i < 0);
    const KeyBinding *b = bindings[mode].table[i];
    if (b) {
        append_binding(s, key, flag, b->cmd_str);
    }
}

static void append_binding_group(String *buf, InputMode mode)
{
    static const char mode_flags[][4] = {
        [INPUT_NORMAL] = "",
        [INPUT_COMMAND] = "-c ",
        [INPUT_SEARCH] = "-s ",
    };

    const char *flag = mode_flags[mode];
    const size_t prev_buf_len = buf->len;

    for (KeyCode k = 0x20; k < 0x7E; k++) {
        append_lookup_table_binding(buf, mode, flag, MOD_CTRL | k);
    }

    for (KeyCode k = 0x20; k < 0x7E; k++) {
        append_lookup_table_binding(buf, mode, flag, MOD_META | k);
    }

    static_assert(MOD_MASK >> MOD_OFFSET == 7);
    for (KeyCode m = 0, modifiers = 0; m <= 7; modifiers = ++m << MOD_OFFSET) {
        for (KeyCode k = KEY_SPECIAL_MIN; k <= KEY_SPECIAL_MAX; k++) {
            append_lookup_table_binding(buf, mode, flag, modifiers | k);
        }
    }

    const PointerArray *array = &bindings[mode].ptr_array;
    size_t n = array->count;
    if (DEBUG && n) {
        // Show a blank line divider in debug mode, to make it easier to
        // see which bindings are in the fallback PointerArray
        string_append_byte(buf, '\n');
    }
    for (size_t i = 0; i < n; i++) {
        const KeyBindingEntry *b = array->ptrs[i];
        append_binding(buf, b->key, flag, b->bind->cmd_str);
    }

    if (buf->len > prev_buf_len) {
        string_append_byte(buf, '\n');
    }
}

String dump_bindings(void)
{
    String buf = string_new(4096);
    for (InputMode mode = 0; mode < ARRAY_COUNT(bindings); mode++) {
        append_binding_group(&buf, mode);
    }
    return buf;
}
