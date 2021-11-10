#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command/args.h"
#include "command/cache.h"
#include "command/macro.h"
#include "command/parse.h"
#include "command/serialize.h"
#include "commands.h"
#include "util/debug.h"
#include "util/intmap.h"
#include "util/macros.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

typedef struct {
    const CommandSet *cmds;
    IntMap map;
} KeyBindingGroup;

static KeyBindingGroup bindings[3] = {
    [INPUT_NORMAL] = {.cmds = &normal_commands},
    [INPUT_COMMAND] = {.cmds = &cmd_mode_commands},
    [INPUT_SEARCH] = {.cmds = &search_mode_commands},
};

void bindings_init(void)
{
    intmap_init(&bindings[INPUT_NORMAL].map, 150);
    intmap_init(&bindings[INPUT_COMMAND].map, 40);
    intmap_init(&bindings[INPUT_SEARCH].map, 40);
}

void add_binding(InputMode mode, KeyCode key, const char *command)
{
    KeyBindingGroup *kbg = &bindings[mode];
    CachedCommand *cc = cached_command_new(kbg->cmds, command);
    cached_command_free(intmap_insert_or_replace(&kbg->map, key, cc));
}

void remove_binding(InputMode mode, KeyCode key)
{
    cached_command_free(intmap_remove(&bindings[mode].map, key));
}

const CachedCommand *lookup_binding(InputMode mode, KeyCode key)
{
    return intmap_get(&bindings[mode].map, key);
}

bool handle_binding(InputMode mode, KeyCode key)
{
    const CachedCommand *binding = lookup_binding(mode, key);
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

void collect_bound_keys(PointerArray *a, const char *prefix)
{
    const IntMap *map = &bindings[INPUT_NORMAL].map;
    for (IntMapIter it = intmap_iter(map); intmap_next(&it); ) {
        const char *str = keycode_to_string(it.entry->key);
        if (str_has_prefix(str, prefix)) {
            ptr_array_append(a, xstrdup(str));
        }
    }
}

typedef struct {
    KeyCode key;
    const char *cmd;
} KeyBinding;

static int binding_cmp(const void *ap, const void *bp)
{
    KeyCode a = ((const KeyBinding*)ap)->key;
    KeyCode b = ((const KeyBinding*)bp)->key;
    return a == b ? 0 : (a > b ? 1 : -1);
}

static bool append_binding_group(String *buf, InputMode mode)
{
    static const char mode_flags[][4] = {
        [INPUT_NORMAL] = "",
        [INPUT_COMMAND] = "-c ",
        [INPUT_SEARCH] = "-s ",
    };

    const IntMap *map = &bindings[mode].map;
    const size_t count = map->count;
    if (unlikely(count == 0)) {
        return false;
    }

    // Clone the contents of the map as an array of key/command pairs
    KeyBinding *array = xnew(*array, count);
    size_t n = 0;
    for (IntMapIter it = intmap_iter(map); intmap_next(&it); ) {
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
    static_assert(ARRAY_COUNT(mode_flags) == ARRAY_COUNT(bindings));
    const char *flag = mode_flags[mode];
    for (size_t i = 0; i < count; i++) {
        string_append_literal(buf, "bind ");
        string_append_cstring(buf, flag);
        string_append_escaped_arg(buf, keycode_to_string(array[i].key), true);
        string_append_byte(buf, ' ');
        string_append_escaped_arg(buf, array[i].cmd, true);
        string_append_byte(buf, '\n');
    }

    free(array);
    return true;
}

String dump_bindings(void)
{
    String buf = string_new(4096);
    for (InputMode i = 0, n = ARRAY_COUNT(bindings); i < n; i++) {
        if (append_binding_group(&buf, i) && i != n - 1) {
            string_append_byte(&buf, '\n');
        }
    }
    return buf;
}
