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
#include "util/debug.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

enum {
    ASCII_RANGE_START = 0x20, // ' '
    ASCII_RANGE_END = 0x7E, // '~'
    ASCII_RANGE_LEN = (ASCII_RANGE_END - ASCII_RANGE_START) + 1,
};

typedef struct {
    KeyCode key;
    KeyBinding *bind;
} KeyBindingEntry;

typedef struct {
    const CommandSet *cmds;

    // Fast lookup table for most common key combinations (Ctrl or Meta
    // with ASCII keys or any combination of modifiers with special keys)
    KeyBinding *table[(2 * ASCII_RANGE_LEN) + (8 * NR_SPECIAL_KEYS)];

    // Fallback for all other keys (Unicode combos etc.)
    PointerArray ptr_array;
} KeyBindingGroup;

static KeyBindingGroup bindings[3];

void bindings_init(void)
{
    // This is done here instead of as a static initializer to allow
    // `bindings` to live in the .bss section and thus avoid placing
    // the whole thing (including the large lookup tables) in the
    // .data section
    bindings[INPUT_NORMAL].cmds = &normal_commands;
    bindings[INPUT_COMMAND].cmds = &cmd_mode_commands;
    bindings[INPUT_SEARCH].cmds = &search_mode_commands;
}

static ssize_t get_lookup_table_index(KeyCode k)
{
    static_assert(ASCII_RANGE_LEN == 95);
    static_assert(MOD_MASK >> MOD_OFFSET == 7);

    const KeyCode modifiers = keycode_get_modifiers(k);
    const KeyCode key = keycode_get_key(k);

    size_t idx = key - KEY_SPECIAL_MIN;
    if (idx < NR_SPECIAL_KEYS) {
        const size_t mod_offset = (modifiers >> MOD_OFFSET) * NR_SPECIAL_KEYS;
        return (2 * ASCII_RANGE_LEN) + mod_offset + idx;
    }

    idx = key - ASCII_RANGE_START;
    if (idx < ASCII_RANGE_LEN) {
        switch (modifiers) {
        case MOD_META:
            idx += ASCII_RANGE_LEN;
            // Fallthrough
        case MOD_CTRL:
            return idx;
        }
    }

    return -1;
}

UNITTEST {
    const size_t size = ARRAY_COUNT(bindings[0].table);
    const KeyCode min = KEY_SPECIAL_MIN;
    const KeyCode max = KEY_SPECIAL_MAX;
    const KeyCode nsk = NR_SPECIAL_KEYS;
    const KeyCode arl = ASCII_RANGE_LEN;
    const KeyCode ax2 = ASCII_RANGE_LEN * 2;
    BUG_ON(size != ax2 + (8 * nsk));

    BUG_ON(get_lookup_table_index(min) != ax2);
    BUG_ON(get_lookup_table_index(max) != ax2 + nsk - 1);
    BUG_ON(get_lookup_table_index(MOD_SHIFT | min) != ax2 + nsk);
    BUG_ON(get_lookup_table_index(MOD_SHIFT | max) != ax2 + (2 * nsk) - 1);
    BUG_ON(get_lookup_table_index(MOD_META | min) != ax2 + (2 * nsk));
    BUG_ON(get_lookup_table_index(MOD_META | max) != ax2 + (3 * nsk) - 1);
    BUG_ON(get_lookup_table_index(MOD_CTRL | min) != ax2 + (4 * nsk));
    BUG_ON(get_lookup_table_index(MOD_CTRL | max) != ax2 + (5 * nsk) - 1);
    BUG_ON(get_lookup_table_index(MOD_MASK | min) != ax2 + (7 * nsk));
    BUG_ON(get_lookup_table_index(MOD_MASK | max) != ax2 + (8 * nsk) - 1);
    BUG_ON(get_lookup_table_index(MOD_MASK | max) != size - 1);

    BUG_ON(get_lookup_table_index(MOD_CTRL | ASCII_RANGE_START) != 0);
    BUG_ON(get_lookup_table_index(MOD_META | ASCII_RANGE_START) != arl);
    BUG_ON(get_lookup_table_index(MOD_CTRL | ASCII_RANGE_END) != arl - 1);
    BUG_ON(get_lookup_table_index(MOD_META | ASCII_RANGE_END) != ax2 - 1);

    BUG_ON(get_lookup_table_index(MOD_CTRL | (ASCII_RANGE_START - 1)) != -1);
    BUG_ON(get_lookup_table_index(MOD_META | (ASCII_RANGE_START - 1)) != -1);
    BUG_ON(get_lookup_table_index(MOD_CTRL | (ASCII_RANGE_END + 1)) != -1);
    BUG_ON(get_lookup_table_index(MOD_META | (ASCII_RANGE_END + 1)) != -1);
    BUG_ON(get_lookup_table_index(MOD_CTRL | MOD_META | 'a') != -1);
    BUG_ON(get_lookup_table_index(MOD_META | 0x0E01) != -1);
}

static KeyBinding *key_binding_new(InputMode mode, const char *cmd_str)
{
    const size_t cmd_str_len = strlen(cmd_str);
    KeyBinding *binding = xmalloc(sizeof(*binding) + cmd_str_len + 1);
    binding->cmd = NULL;
    memcpy(binding->cmd_str, cmd_str, cmd_str_len + 1);

    const CommandSet *cmds = bindings[mode].cmds;
    PointerArray array = PTR_ARRAY_INIT;
    if (parse_commands(cmds, &array, cmd_str) != CMDERR_NONE) {
        goto nocache;
    }

    ptr_array_trim_nulls(&array);
    size_t n = array.count;
    if (n < 2 || ptr_array_idx(&array, NULL) != n - 1) {
        // Only single commands can be cached
        goto nocache;
    }

    const Command *cmd = cmds->lookup(array.ptrs[0]);
    if (!cmd) {
        // Aliases or non-existent commands can't be cached
        goto nocache;
    }

    if (memchr(cmd_str, '$', cmd_str_len)) {
        // Commands containing variables can't be cached
        goto nocache;
    }

    free(ptr_array_remove_idx(&array, 0));
    CommandArgs cmdargs = {.args = (char**)array.ptrs};
    if (do_parse_args(cmd, &cmdargs) != 0) {
        goto nocache;
    }

    // Command can be cached; binding takes ownership of args array
    binding->cmd = cmd;
    binding->a = cmdargs;
    return binding;

nocache:
    ptr_array_free(&array);
    return binding;
}

static void free_key_binding(KeyBinding *binding)
{
    if (binding) {
        if (binding->cmd) {
            free_string_array(binding->a.args);
        }
        free(binding);
    }
}

static void free_key_binding_entry(KeyBindingEntry *entry)
{
    free_key_binding(entry->bind);
    free(entry);
}

void add_binding(InputMode mode, KeyCode key, const char *command)
{
    const ssize_t idx = get_lookup_table_index(key);
    if (likely(idx >= 0)) {
        KeyBinding **table = bindings[mode].table;
        free_key_binding(table[idx]);
        table[idx] = key_binding_new(mode, command);
        return;
    }

    KeyBindingEntry *entry = xnew(KeyBindingEntry, 1);
    entry->key = key;
    entry->bind = key_binding_new(mode, command);
    ptr_array_append(&bindings[mode].ptr_array, entry);
}

void remove_binding(InputMode mode, KeyCode key)
{
    const ssize_t idx = get_lookup_table_index(key);
    if (likely(idx >= 0)) {
        KeyBinding **table = bindings[mode].table;
        free_key_binding(table[idx]);
        table[idx] = NULL;
        return;
    }

    PointerArray *ptr_array = &bindings[mode].ptr_array;
    size_t i = ptr_array->count;
    while (i > 0) {
        KeyBindingEntry *entry = ptr_array->ptrs[--i];
        if (entry->key == key) {
            ptr_array_remove_idx(ptr_array, i);
            free_key_binding_entry(entry);
            return;
        }
    }
}

const KeyBinding *lookup_binding(InputMode mode, KeyCode key)
{
    const ssize_t idx = get_lookup_table_index(key);
    if (likely(idx >= 0)) {
        return bindings[mode].table[idx];
    }

    PointerArray *ptr_array = &bindings[mode].ptr_array;
    for (size_t i = ptr_array->count; i > 0; i--) {
        KeyBindingEntry *entry = ptr_array->ptrs[i - 1];
        if (entry->key == key) {
            return entry->bind;
        }
    }

    return NULL;
}

bool handle_binding(InputMode mode, KeyCode key)
{
    const KeyBinding *binding = lookup_binding(mode, key);
    if (!binding) {
        return false;
    }

    // If the command isn't cached or a macro is being recorded
    if (!binding->cmd || macro_is_recording()) {
        // Parse and run command string
        const CommandSet *cmds = bindings[mode].cmds;
        handle_command(cmds, binding->cmd_str, !!cmds->allow_recording);
        return true;
    }

    // Command is cached; call it directly
    begin_change(CHANGE_MERGE_NONE);
    current_command = binding->cmd;
    binding->cmd->cmd(&binding->a);
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
    for (KeyCode k = ASCII_RANGE_START; k <= ASCII_RANGE_END; k++) {
        maybe_add_lt_key_completion(prefix, MOD_CTRL | k);
    }

    for (KeyCode k = ASCII_RANGE_START; k <= ASCII_RANGE_END; k++) {
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
        const KeyBindingEntry *entry = array->ptrs[i];
        maybe_add_key_completion(prefix, entry->key);
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
    const KeyBinding *binding = bindings[mode].table[i];
    if (binding) {
        append_binding(s, key, flag, binding->cmd_str);
    }
}

static void append_binding_group(String *buf, InputMode mode)
{
    static const char mode_flags[][4] = {
        [INPUT_NORMAL] = "",
        [INPUT_COMMAND] = "-c ",
        [INPUT_SEARCH] = "-s ",
    };

    static_assert(ARRAY_COUNT(mode_flags) == ARRAY_COUNT(bindings));
    const char *flag = mode_flags[mode];
    const size_t prev_buf_len = buf->len;

    for (KeyCode k = ASCII_RANGE_START; k <= ASCII_RANGE_END; k++) {
        append_lookup_table_binding(buf, mode, flag, MOD_CTRL | k);
    }

    for (KeyCode k = ASCII_RANGE_START; k <= ASCII_RANGE_END; k++) {
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
        const KeyBindingEntry *entry = array->ptrs[i];
        append_binding(buf, entry->key, flag, entry->bind->cmd_str);
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
