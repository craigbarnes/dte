#include <stdlib.h>
#include "alias.h"
#include "util/xmalloc.h"

void add_alias(HashMap *aliases, const char *name, const char *value)
{
    free(hashmap_insert_or_replace(aliases, xstrdup(name), xstrdup(value)));
}

void remove_alias(HashMap *aliases, const char *name)
{
    free(hashmap_remove(aliases, name));
}
