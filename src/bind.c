#include <limits.h>
#include <stdlib.h>
#include "bind.h"
#include "change.h"
#include "command/macro.h"
#include "command/run.h"
#include "command/serialize.h"
#include "util/debug.h"
#include "util/xmalloc.h"

void add_binding(IntMap *bindings, KeyCode key, CachedCommand *cc)
{
    cached_command_free(intmap_insert_or_replace(bindings, key, cc));
}

void remove_binding(IntMap *bindings, KeyCode key)
{
    cached_command_free(intmap_remove(bindings, key));
}

const CachedCommand *lookup_binding(const IntMap *bindings, KeyCode key)
{
    return intmap_get(bindings, key);
}

void free_bindings(IntMap *bindings)
{
    intmap_free(bindings, (FreeFunction)cached_command_free);
}

bool handle_binding(EditorState *e, InputMode mode, KeyCode key)
{
    const IntMap *bindings = &e->modes[mode].key_bindings;
    const CachedCommand *binding = lookup_binding(bindings, key);
    if (!binding) {
        return false;
    }

    // If the command isn't cached or a macro is being recorded
    const CommandSet *cmds = e->modes[mode].cmds;
    if (!binding->cmd || (cmds->macro_record && macro_is_recording(&e->macro))) {
        // Parse and run command string
        CommandRunner runner = cmdrunner_for_mode(e, mode, true);
        return handle_command(&runner, binding->cmd_str);
    }

    // Command is cached; call it directly
    begin_change(CHANGE_MERGE_NONE);
    current_command = binding->cmd;
    bool r = binding->cmd->cmd(e, &binding->a);
    current_command = NULL;
    end_change();
    return r;
}

typedef struct {
    KeyCode key;
    const char *cmd;
} KeyBinding;

static int binding_cmp(const void *ap, const void *bp)
{
    static_assert((MOD_MASK | KEY_SPECIAL_MAX) <= INT_MAX);
    const KeyBinding *a = ap;
    const KeyBinding *b = bp;
    return (int)a->key - (int)b->key;
}

UNITTEST {
    KeyBinding a = {.key = KEY_F5};
    KeyBinding b = {.key = KEY_F5};
    BUG_ON(binding_cmp(&a, &b) != 0);
    b.key = KEY_F3;
    BUG_ON(binding_cmp(&a, &b) <= 0);
    b.key = KEY_F12;
    BUG_ON(binding_cmp(&a, &b) >= 0);
}

bool dump_bindings(const IntMap *bindings, const char *flag, String *buf)
{
    const size_t count = bindings->count;
    if (unlikely(count == 0)) {
        return false;
    }

    // Clone the contents of the map as an array of key/command pairs
    KeyBinding *array = xnew(*array, count);
    size_t n = 0;
    for (IntMapIter it = intmap_iter(bindings); intmap_next(&it); ) {
        const CachedCommand *cc = it.entry->value;
        array[n++] = (KeyBinding) {
            .key = it.entry->key,
            .cmd = cc->cmd_str,
        };
    }

    // Sort the array
    BUG_ON(n != count);
    qsort(array, count, sizeof(array[0]), binding_cmp);

    // Serialize the bindings in sorted order
    char keystr[KEYCODE_STR_MAX];
    for (size_t i = 0; i < count; i++) {
        string_append_literal(buf, "bind ");
        string_append_cstring(buf, flag);
        size_t keylen = keycode_to_string(array[i].key, keystr);
        string_append_escaped_arg_sv(buf, string_view(keystr, keylen), true);
        string_append_byte(buf, ' ');
        string_append_escaped_arg(buf, array[i].cmd, true);
        string_append_byte(buf, '\n');
    }

    free(array);
    return true;
}
