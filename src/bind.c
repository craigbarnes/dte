#include "bind.h"
#include "common.h"
#include "error.h"
#include "command.h"
#include "ptr-array.h"

typedef struct {
    Key keys[3];
    size_t count;
} KeyChain;

typedef struct {
    char *command;
    KeyChain chain;
} Binding;

static KeyChain pressed_keys;
static PointerArray bindings = PTR_ARRAY_INIT;

static bool parse_keys(KeyChain *chain, const char *str)
{
    char *keys = xstrdup(str);
    size_t len = strlen(keys);

    // Convert all whitespace to \0
    for (size_t i = 0; i < len; i++) {
        if (ascii_isspace(keys[i])) {
            keys[i] = 0;
        }
    }

    memzero(chain);
    for (size_t i = 0; i < len; ) {
        while (i < len && keys[i] == 0) {
            i++;
        }
        const char *key = keys + i;
        while (i < len && keys[i] != 0) {
            i++;
        }
        if (key == keys + i) {
            break;
        }

        if (chain->count >= ARRAY_COUNT(chain->keys)) {
            error_msg("Too many keys.");
            free(keys);
            return false;
        }
        if (!parse_key(&chain->keys[chain->count], key)) {
            error_msg("Invalid key %s", key);
            free(keys);
            return false;
        }
        chain->count++;
    }
    free(keys);
    if (chain->count == 0) {
        error_msg("Empty key not allowed.");
        return false;
    }
    return true;
}

void add_binding(const char *keys, const char *command)
{
    Binding *b = xnew(Binding, 1);
    if (!parse_keys(&b->chain, keys)) {
        free(b);
        return;
    }

    b->command = xstrdup(command);
    ptr_array_add(&bindings, b);
}

void remove_binding(const char *keys)
{
    KeyChain chain;
    size_t i = bindings.count;

    if (!parse_keys(&chain, keys)) {
        return;
    }

    while (i > 0) {
        Binding *b = bindings.ptrs[--i];

        if (memcmp(&b->chain, &chain, sizeof(chain)) == 0) {
            ptr_array_remove_idx(&bindings, i);
            free(b->command);
            free(b);
            return;
        }
    }
}

void handle_binding(Key key)
{
    pressed_keys.keys[pressed_keys.count] = key;
    pressed_keys.count++;

    for (size_t i = bindings.count; i > 0; i--) {
        Binding *b = bindings.ptrs[i - 1];
        KeyChain *c = &b->chain;

        if (c->count < pressed_keys.count) {
            continue;
        }

        if (memcmp (
            c->keys,
            pressed_keys.keys,
            pressed_keys.count * sizeof(pressed_keys.keys[0])
        )) {
            continue;
        }

        if (c->count > pressed_keys.count) {
            return;
        }

        handle_command(commands, b->command);
        break;
    }
    memzero(&pressed_keys);
}

int nr_pressed_keys(void)
{
    return pressed_keys.count;
}
