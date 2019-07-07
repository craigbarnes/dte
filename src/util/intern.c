#include "intern.h"
#include "hashset.h"

const void *mem_intern(const void *data, size_t len)
{
    static HashSet pool;
    if (!pool.table_size) {
        hashset_init(&pool, 32, false);
    }

    HashSetEntry *e = hashset_get(&pool, data, len);
    if (!e) {
        e = hashset_add(&pool, data, len);
    }
    return e->str;
}
