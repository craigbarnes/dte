#include "intern.h"
#include "hashset.h"

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
static HashSet interned_strings;

const void *mem_intern(const void *data, size_t len)
{
    if (unlikely(interned_strings.table_size == 0)) {
        hashset_init(&interned_strings, 32, false);
    }

    HashSetEntry *e = hashset_insert(&interned_strings, data, len);
    return e->str;
}

bool mem_is_intern(const void *data, size_t len)
{
    const HashSetEntry *entry = hashset_get(&interned_strings, data, len);
    return entry && entry->str == data;
}

void free_interned_strings(void)
{
    hashset_free(&interned_strings);
}
