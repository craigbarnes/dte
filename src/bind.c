#include <limits.h>
#include <stdlib.h>
#include "bind.h"
#include "change.h"
#include "command/macro.h"
#include "command/run.h"
#include "command/serialize.h"
#include "editor.h"
#include "util/debug.h"
#include "util/xmalloc.h"

/**
 * Adds or replaces a key binding in the provided map.
 * Takes ownership of the CachedCommand.
 */
void add_binding(IntMap *bindings, KeyCode key, CachedCommand *cc)
{
    cached_command_free(intmap_insert_or_replace(bindings, key, cc));
}

/**
 * Removes a binding and frees its associated resources.
 */
void remove_binding(IntMap *bindings, KeyCode key)
{
    cached_command_free(intmap_remove(bindings, key));
}

/**
 * Retrieves a binding without transferring ownership.
 */
const CachedCommand *lookup_binding(const IntMap *bindings, KeyCode key)
{
    return intmap_get(bindings, key);
}

/**
 * Clears all bindings and frees the map resources.
 */
void free_bindings(IntMap *bindings)
{
    intmap_free(bindings, FREE_FUNC(cached_command_free));
}

/**
 * Processes a key press based on current mode bindings.
 */
bool handle_binding(EditorState *e, const ModeHandler *handler, KeyCode key)
{
    const CachedCommand *binding = lookup_binding(&handler->key_bindings, key);
    if (!binding) {
        return false;
    }

    const CommandSet *cmds = handler->cmds;
    BUG_ON(!cmds);

    /* * Re-evaluate the command if it's not cached OR if we are currently 
     * recording a macro and the handler allows it.
     */
    bool should_reparse = !binding->cmd || 
                         (cmds->macro_record && macro_is_recording(&e->macro));

    if (should_reparse) {
        CommandRunner runner = cmdrunner(e, cmds);
        runner.flags |= CMDRUNNER_ALLOW_RECORDING;
        return handle_command(&runner, binding->cmd_str);
    }

    /* Directly execute the cached function to optimize latency */
    begin_change(CHANGE_MERGE_NONE);
    command_func_call(e, &e->err, binding->cmd, &binding->a);
    end_change();
    
    return true;
}

typedef struct {
    KeyCode key;
    const char *cmd;
} KeyBinding;

/**
 * Comparator for sorting bindings by KeyCode value.
 */
static int binding_cmp(const void *ap, const void *bp)
{
    /* Modified to match your project's static_assert macro (1 argument only) */
    static_assert((MOD_MASK | KEY_SPECIAL_MAX) <= INT_MAX);
    
    const KeyBinding *a = ap;
    const KeyBinding *b = bp;
    return (a->key > b->key) - (a->key < b->key);
}

UNITTEST {
    /* NOLINTBEGIN(bugprone-assert-side-effect) */
    KeyBinding a = {.key = KEY_F5};
    KeyBinding b = {.key = KEY_F5};
    BUG_ON(binding_cmp(&a, &b) != 0);
    b.key = KEY_F3;
    BUG_ON(binding_cmp(&a, &b) <= 0);
    b.key = KEY_F12;
    BUG_ON(binding_cmp(&a, &b) >= 0);
    /* NOLINTEND(bugprone-assert-side-effect) */
}

/**
 * Serializes all active bindings into a String buffer for persistence or display.
 */
bool dump_bindings(const IntMap *bindings, const char *flag, String *buf)
{
    const size_t count = bindings->count;
    if (unlikely(count == 0)) {
        return false;
    }

    /* Allocate temporary array to sort the key/command pairs */
    KeyBinding *array = xmallocarray(count, sizeof(*array));
    size_t n = 0;
    
    for (IntMapIter it = intmap_iter(bindings); intmap_next(&it); ) {
        const CachedCommand *cc = it.entry->value;
        array[n++] = (KeyBinding) {
            .key = it.entry->key,
            .cmd = cc->cmd_str,
        };
    }

    BUG_ON(n != count);
    qsort(array, count, sizeof(array[0]), binding_cmp);

    /* Serialize bindings in a stable, sorted order */
    char keystr[KEYCODE_STR_BUFSIZE];
    for (size_t i = 0; i < count; i++) {
        string_append_literal(buf, "bind ");
        
        /* Handle mode/table flags (-T) */
        if (flag[0] != '\0' && flag[0] != '-') {
            string_append_literal(buf, "-T ");
            string_append_escaped_arg(buf, flag, true);
            string_append_byte(buf, ' ');
        } else {
            string_append_cstring(buf, flag);
        }

        size_t keylen = keycode_to_str(array[i].key, keystr);
        string_append_escaped_arg_sv(buf, string_view(keystr, keylen), true);
        string_append_byte(buf, ' ');
        string_append_escaped_arg(buf, array[i].cmd, true);
        string_append_byte(buf, '\n');
    }

    free(array);
    return true;
}
