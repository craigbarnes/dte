#include "intern.h"
#include "hashset.h"

static HashSet intern_pool;

const void *mem_intern(const void *data, size_t len)
{
    if (unlikely(intern_pool.table_size == 0)) {
        hashset_init(&intern_pool, 32, false);
    }

    HashSetEntry *e = hashset_add(&intern_pool, data, len);
    return e->str;
}

void free_intern_pool(void)
{
    hashset_free(&intern_pool);
}
