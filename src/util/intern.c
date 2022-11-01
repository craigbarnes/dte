#include "intern.h"
#include "hashset.h"

static HashSet interned_strings;

const void *mem_intern(const void *data, size_t len)
{
    if (unlikely(interned_strings.table_size == 0)) {
        hashset_init(&interned_strings, 32, false);
    }

    HashSetEntry *e = hashset_add(&interned_strings, data, len);
    return e->str;
}

void free_interned_strings(void)
{
    hashset_free(&interned_strings);
}
