#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "alias.h"
#include "command.h"
#include "completion.h"
#include "editor.h"
#include "error.h"
#include "util/ascii.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
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

static bool is_valid_alias_name(const char *name)
{
    if (unlikely(name[0] == '\0')) {
        error_msg("Empty alias name not allowed");
        return false;
    }

    for (unsigned char c; (c = *name); name++) {
        if (is_word_byte(c) || c == '-' || c == '?' || c == '!') {
            continue;
        }
        error_msg("Invalid byte in alias name: 0x%02x", (unsigned int)c);
        return false;
    }

    return true;
}

void add_alias(const char *name, const char *value)
{
    if (!is_valid_alias_name(name)) {
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

static int alias_cmp(const void *ap, const void *bp)
{
    const CommandAlias *const *a = ap;
    const CommandAlias *const *b = bp;
    return strcmp((*a)->name, (*b)->name);
}

void sort_aliases(void)
{
    ptr_array_sort(&aliases, alias_cmp);
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

String dump_aliases(void)
{
    String buf = string_new(4096);
    for (size_t i = 0; i < aliases.count; i++) {
        const CommandAlias *alias = aliases.ptrs[i];
        string_sprintf(&buf, " %s  ->  %s\n", alias->name, alias->value);
    }
    return buf;
}
