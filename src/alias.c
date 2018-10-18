#include <stdbool.h>
#include "alias.h"
#include "command.h"
#include "common.h"
#include "completion.h"
#include "editor.h"
#include "error.h"
#include "util/ascii.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/xmalloc.h"

typedef struct {
    char *name;
    char *value;
} CommandAlias;

static PointerArray aliases = PTR_ARRAY_INIT;

static void CONSTRUCTOR prealloc(void)
{
    ptr_array_init(&aliases, 32);
}

static bool is_valid_alias_name(const char *const name)
{
    for (size_t i = 0; name[i]; i++) {
        const char ch = name[i];
        if (!is_alnum_or_underscore(ch) && ch != '-') {
            return false;
        }
    }
    return !!name[0];
}

void add_alias(const char *name, const char *value)
{
    if (!is_valid_alias_name(name)) {
        error_msg("Invalid alias name '%s'", name);
        return;
    }
    if (find_command(commands, name)) {
        error_msg("Can't replace existing command %s with an alias", name);
        return;
    }

    // Replace existing alias
    for (size_t i = 0; i < aliases.count; i++) {
        CommandAlias *alias = aliases.ptrs[i];
        if (streq(alias->name, name)) {
            free(alias->value);
            alias->value = xstrdup(value);
            return;
        }
    }

    CommandAlias *alias = xnew(CommandAlias, 1);
    alias->name = xstrdup(name);
    alias->value = xstrdup(value);
    ptr_array_add(&aliases, alias);

    if (editor.status != EDITOR_INITIALIZING) {
        sort_aliases();
    }
}

static int alias_cmp(const void *const ap, const void *const bp)
{
    const CommandAlias *a = *(const CommandAlias **)ap;
    const CommandAlias *b = *(const CommandAlias **)bp;
    return strcmp(a->name, b->name);
}

void sort_aliases(void)
{
    ptr_array_sort(aliases, alias_cmp);
}

const char *find_alias(const char *const name)
{
    const CommandAlias key = {.name = (char*) name};
    const void *ptr = ptr_array_bsearch(aliases, &key, alias_cmp);
    if (ptr) {
        const CommandAlias *alias = *(const CommandAlias **) ptr;
        return alias->value;
    }
    return NULL;
}

void collect_aliases(const char *const prefix)
{
    for (size_t i = 0; i < aliases.count; i++) {
        const CommandAlias *const alias = aliases.ptrs[i];
        if (str_has_prefix(alias->name, prefix)) {
            add_completion(xstrdup(alias->name));
        }
    }
}
