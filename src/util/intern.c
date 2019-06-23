#include "intern.h"
#include "hash.h"
#include "hashmap.h"
#include "xmalloc.h"

typedef struct {
    HashMapEntry entry;
    size_t len;
    char data[];
} StringInternEntry;

static int entry_cmp (
    const void* UNUSED_ARG(cmp_data),
    const StringInternEntry *e1,
    const StringInternEntry *e2,
    const char *keydata
) {
    return
        e1->data != keydata
        && (e1->len != e2->len || memcmp(e1->data, keydata, e1->len));
}

const void *mem_intern(const void *data, size_t len)
{
    static HashMap pool;
    if (!pool.tablesize) {
        hashmap_init(&pool, (HashMapCompareFunction)entry_cmp, NULL, 0);
    }

    StringInternEntry key;
    hashmap_entry_init(&key, fnv_1a_hash(data, len));
    key.len = len;

    StringInternEntry *e = hashmap_get(&pool, &key, data);
    if (!e) {
        e = xmalloc(sizeof(StringInternEntry) + len + 1);
        memcpy(e->data, data, len);
        e->data[len] = '\0';
        hashmap_entry_init(e, key.entry.hash);
        e->len = len;
        hashmap_add(&pool, e);
    }

    return e->data;
}
