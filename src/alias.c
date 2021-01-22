#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "alias.h"
#include "command/serialize.h"
#include "completion.h"
#include "editor.h"
#include "util/hashmap.h"
#include "util/macros.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static HashMap aliases = HASHMAP_INIT;

void init_aliases(void)
{
    BUG_ON(aliases.entries);
    hashmap_init(&aliases, 32);
}

void add_alias(const char *name, const char *value)
{
    char *value_copy = xstrdup(value);
    HashMapEntry *e = hashmap_find(&aliases, name);
    if (e) {
        // Alias exists; just update the value
        free(e->value);
        e->value = value_copy;
        return;
    }

    hashmap_insert(&aliases, xstrdup(name), value_copy);
}

const char *find_alias(const char *const name)
{
    return hashmap_get(&aliases, name);
}

void collect_aliases(const char *const prefix)
{
    for (HashMapIter it = hashmap_iter(&aliases); hashmap_next(&it); ) {
        const char *name = it.entry->key;
        if (str_has_prefix(name, prefix)) {
            add_completion(xstrdup(name));
        }
    }
}

typedef struct {
    const char *name;
    const char *value;
} CommandAlias;

static int alias_cmp(const void *ap, const void *bp)
{
    const CommandAlias *a = ap;
    const CommandAlias *b = bp;
    return strcmp(a->name, b->name);
}

String dump_aliases(void)
{
    const size_t count = aliases.count;
    CommandAlias *array = xnew(CommandAlias, count);

    // Clone the contents of the HashMap as an array of name/value pairs
    size_t n = 0;
    for (HashMapIter it = hashmap_iter(&aliases); hashmap_next(&it); ) {
        array[n++] = (CommandAlias) {
            .name = it.entry->key,
            .value = it.entry->value,
        };
    }

    // Sort the array
    BUG_ON(n != count);
    qsort(array, count, sizeof(CommandAlias), alias_cmp);

    // Serialize the aliases in sorted order
    String buf = string_new(4096);
    for (size_t i = 0; i < count; i++) {
        string_append_literal(&buf, "alias ");
        string_append_escaped_arg(&buf, array[i].name, false);
        string_append_byte(&buf, ' ');
        string_append_escaped_arg(&buf, array[i].value, false);
        string_append_byte(&buf, '\n');
    }

    free(array);
    return buf;
}
