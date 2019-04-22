#include <sys/types.h>
#include "bind.h"
#include "command.h"
#include "common.h"
#include "debug.h"
#include "error.h"
#include "util/ascii.h"
#include "util/ptr-array.h"
#include "util/xmalloc.h"

typedef struct {
    char *command;
    KeyCode key;
} Binding;

// Fast lookup table for most common key combinations (Ctrl or Meta
// with ASCII keys or any combination of modifiers with special keys)
static char *bindings_lookup_table[(2 * 128) + (8 * NR_SPECIAL_KEYS)];

// Fallback for all other keys (Unicode combos etc.)
static PointerArray bindings_ptr_array = PTR_ARRAY_INIT;

static CONST_FN ssize_t key_lookup_index(KeyCode k)
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
    BUG_ON(key_lookup_index(MOD_MASK | KEY_SPECIAL_MAX) != 256 + (8 * NR_SPECIAL_KEYS) - 1);
    BUG_ON(key_lookup_index(KEY_SPECIAL_MIN) != 256);
    BUG_ON(key_lookup_index(KEY_SPECIAL_MAX) != 256 + NR_SPECIAL_KEYS - 1);
    BUG_ON(key_lookup_index(MOD_CTRL | KEY_SPECIAL_MIN) != 256 + NR_SPECIAL_KEYS);
    BUG_ON(key_lookup_index(MOD_SHIFT | KEY_SPECIAL_MAX) != 256 + (5 * NR_SPECIAL_KEYS) - 1);

    BUG_ON(key_lookup_index(MOD_CTRL | ' ') != 32);
    BUG_ON(key_lookup_index(MOD_META | ' ') != 32 + 128);
    BUG_ON(key_lookup_index(MOD_CTRL | '~') != 126);
    BUG_ON(key_lookup_index(MOD_META | '~') != 126 + 128);

    BUG_ON(key_lookup_index(MOD_CTRL | MOD_META | 'a') != -1);
    BUG_ON(key_lookup_index(MOD_META | 0x0E01) != -1);
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
        char *old_command = bindings_lookup_table[idx];
        if (old_command) {
            free(old_command);
        }
        bindings_lookup_table[idx] = xstrdup(command);
        return;
    }

    Binding *b = xnew(Binding, 1);
    b->key = key;
    b->command = xstrdup(command);
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
        char *command = bindings_lookup_table[idx];
        if (command) {
            free(command);
            bindings_lookup_table[idx] = NULL;
        }
        return;
    }

    size_t i = bindings_ptr_array.count;
    while (i > 0) {
        Binding *b = bindings_ptr_array.ptrs[--i];
        if (b->key == key) {
            ptr_array_remove_idx(&bindings_ptr_array, i);
            free(b->command);
            free(b);
            return;
        }
    }
}

void handle_binding(KeyCode key)
{
    const ssize_t idx = key_lookup_index(key);
    if (idx >= 0) {
        const char *command = bindings_lookup_table[idx];
        if (command) {
            handle_command(commands, command);
            return;
        }
    }

    for (size_t i = bindings_ptr_array.count; i > 0; i--) {
        Binding *b = bindings_ptr_array.ptrs[i - 1];
        if (b->key == key) {
            handle_command(commands, b->command);
            break;
        }
    }
}

String dump_bindings(void)
{
    String buf = STRING_INIT;

    for (KeyCode k = 0x20; k < 0x7E; k++) {
        const char *command = bindings_lookup_table[k];
        if (command) {
            const char *keystr = key_to_string(MOD_CTRL | k);
            string_sprintf(&buf, "   %-10s  %s\n", keystr, command);
        }
    }

    for (KeyCode k = 0x20; k < 0x7E; k++) {
        const char *command = bindings_lookup_table[k + 128];
        if (command) {
            const char *keystr = key_to_string(MOD_META | k);
            string_sprintf(&buf, "   %-10s  %s\n", keystr, command);
        }
    }

    static_assert(MOD_CTRL == (1 << 24));
    for (KeyCode m = 0; m <= 7; m++) {
        const KeyCode modifiers = m << 24;
        for (size_t k = KEY_SPECIAL_MIN; k <= KEY_SPECIAL_MAX; k++) {
            const size_t mod_offset = m * NR_SPECIAL_KEYS;
            const size_t i = (2 * 128) + mod_offset + (k - KEY_SPECIAL_MIN);
            const char *command = bindings_lookup_table[i];
            if (command) {
                const char *keystr = key_to_string(modifiers | k);
                string_sprintf(&buf, "   %-10s  %s\n", keystr, command);
            }
        }
    }

    for (size_t i = 0, nbinds = bindings_ptr_array.count; i < nbinds; i++) {
        const Binding *b = bindings_ptr_array.ptrs[i];
        const char *keystr = key_to_string(b->key);
        string_sprintf(&buf, "   %-10s  %s\n", keystr, b->command);
    }

    return buf;
}
