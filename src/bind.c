#include <sys/types.h>
#include "bind.h"
#include "change.h"
#include "command.h"
#include "debug.h"
#include "error.h"
#include "parse-args.h"
#include "util/ascii.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

typedef struct {
    KeyCode key;
    KeyBinding *bind;
} KeyBindingEntry;

// Fast lookup table for most common key combinations (Ctrl or Meta
// with ASCII keys or any combination of modifiers with special keys)
static KeyBinding *bindings_lookup_table[(2 * 128) + (8 * NR_SPECIAL_KEYS)];

// Fallback for all other keys (Unicode combos etc.)
static PointerArray bindings_ptr_array = PTR_ARRAY_INIT;

static ssize_t key_lookup_index(KeyCode k)
{
    const KeyCode modifiers = keycode_get_modifiers(k);
    const KeyCode key = keycode_get_key(k);

    static_assert(MOD_MASK >> 24 == (1 | 2 | 4));

    if (key >= KEY_SPECIAL_MIN && key <= KEY_SPECIAL_MAX) {
        const size_t mod_offset = (modifiers >> 24) * NR_SPECIAL_KEYS;
        return (2 * 128) + mod_offset + (key - KEY_SPECIAL_MIN);
    }

    if (key >= 0x20 && key <= 0x7E) {
        switch (modifiers) {
        case MOD_CTRL:
            return key;
        case MOD_META:
            return key + 128;
        default:
            break;
        }
    }

    return -1;
}

UNITTEST {
    const size_t size = ARRAY_COUNT(bindings_lookup_table);
    const KeyCode min = KEY_SPECIAL_MIN;
    const KeyCode max = KEY_SPECIAL_MAX;
    const KeyCode nsk = NR_SPECIAL_KEYS;
    BUG_ON(key_lookup_index(MOD_MASK | max) != size - 1);
    BUG_ON(key_lookup_index(min) != 256);
    BUG_ON(key_lookup_index(max) != 256 + nsk - 1);
    BUG_ON(key_lookup_index(MOD_CTRL | min) != 256 + nsk);
    BUG_ON(key_lookup_index(MOD_SHIFT | max) != 256 + (5 * nsk) - 1);

    BUG_ON(key_lookup_index(MOD_CTRL | ' ') != 32);
    BUG_ON(key_lookup_index(MOD_META | ' ') != 32 + 128);
    BUG_ON(key_lookup_index(MOD_CTRL | '~') != 126);
    BUG_ON(key_lookup_index(MOD_META | '~') != 126 + 128);

    BUG_ON(key_lookup_index(MOD_CTRL | MOD_META | 'a') != -1);
    BUG_ON(key_lookup_index(MOD_META | 0x0E01) != -1);
}

static KeyBinding *key_binding_new(const char *cmd_str)
{
    const size_t cmd_str_len = strlen(cmd_str);
    KeyBinding *b = xmalloc(sizeof(*b) + cmd_str_len + 1);
    b->cmd = NULL;
    memcpy(b->cmd_str, cmd_str, cmd_str_len + 1);

    PointerArray array = PTR_ARRAY_INIT;
    CommandParseError err = 0;
    if (!parse_commands(&array, cmd_str, &err)) {
        goto out;
    }

    ptr_array_trim_nulls(&array);
    if (array.count < 2 || ptr_array_idx(&array, NULL) != (array.count - 1)) {
        // Only single commands can be cached
        goto out;
    }

    const Command *cmd = find_command(commands, array.ptrs[0]);
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
    suppress_error_msg();
    bool ok = parse_args(cmd, &a);
    unsuppress_error_msg();
    if (!ok) {
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

void add_binding(const char *keystr, const char *command)
{
    KeyCode key;
    if (!parse_key(&key, keystr)) {
        error_msg("invalid key string: %s", keystr);
        return;
    }

    const ssize_t idx = key_lookup_index(key);
    if (idx >= 0) {
        key_binding_free(bindings_lookup_table[idx]);
        bindings_lookup_table[idx] = key_binding_new(command);
        return;
    }

    KeyBindingEntry *b = xnew(KeyBindingEntry, 1);
    b->key = key;
    b->bind = key_binding_new(command);
    ptr_array_add(&bindings_ptr_array, b);
}

void remove_binding(const char *keystr)
{
    KeyCode key;
    if (!parse_key(&key, keystr)) {
        return;
    }

    const ssize_t idx = key_lookup_index(key);
    if (idx >= 0) {
        key_binding_free(bindings_lookup_table[idx]);
        bindings_lookup_table[idx] = NULL;
        return;
    }

    size_t i = bindings_ptr_array.count;
    while (i > 0) {
        KeyBindingEntry *b = bindings_ptr_array.ptrs[--i];
        if (b->key == key) {
            ptr_array_remove_idx(&bindings_ptr_array, i);
            key_binding_free(b->bind);
            free(b);
            return;
        }
    }
}

const KeyBinding *lookup_binding(KeyCode key)
{
    const ssize_t idx = key_lookup_index(key);
    if (idx >= 0) {
        const KeyBinding *b = bindings_lookup_table[idx];
        if (b) {
            return b;
        }
    }

    for (size_t i = bindings_ptr_array.count; i > 0; i--) {
        KeyBindingEntry *b = bindings_ptr_array.ptrs[i - 1];
        if (b->key == key) {
            return b->bind;
        }
    }

    return NULL;
}

void handle_binding(KeyCode key)
{
    const KeyBinding *b = lookup_binding(key);
    if (!b) {
        return;
    }

    if (!b->cmd) {
        // Command isn't cached; parse and run command string
        handle_command(commands, b->cmd_str);
        return;
    }

    // Command is cached; call it directly
    begin_change(CHANGE_MERGE_NONE);
    b->cmd->cmd(&b->a);
    end_change();
}

static void append_lookup_table_binding(String *buf, KeyCode key)
{
    const ssize_t i = key_lookup_index(key);
    BUG_ON(i < 0);
    const KeyBinding *b = bindings_lookup_table[i];
    if (b) {
        const char *keystr = key_to_string(key);
        string_sprintf(buf, "   %-10s  %s\n", keystr, b->cmd_str);
    }
}

String dump_bindings(void)
{
    String buf = string_new(4096);

    for (KeyCode k = 0x20; k < 0x7E; k++) {
        append_lookup_table_binding(&buf, MOD_CTRL | k);
    }

    for (KeyCode k = 0x20; k < 0x7E; k++) {
        append_lookup_table_binding(&buf, MOD_META | k);
    }

    static_assert(MOD_CTRL == (1 << 24));
    for (KeyCode m = 0, modifiers = 0; m <= 7; modifiers = ++m << 24) {
        for (KeyCode k = KEY_SPECIAL_MIN; k <= KEY_SPECIAL_MAX; k++) {
            append_lookup_table_binding(&buf, modifiers | k);
        }
    }

    for (size_t i = 0, nbinds = bindings_ptr_array.count; i < nbinds; i++) {
        const KeyBindingEntry *b = bindings_ptr_array.ptrs[i];
        const char *keystr = key_to_string(b->key);
        string_sprintf(&buf, "   %-10s  %s\n", keystr, b->bind->cmd_str);
    }

    return buf;
}
