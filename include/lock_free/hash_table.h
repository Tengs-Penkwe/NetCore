#ifndef __LOCK_FREE_HASH_TABLE_H__
#define __LOCK_FREE_HASH_TABLE_H__

#include <common.h>      // BEGIN, END DECLS
#include "liblfds711.h"  // Lock-free structures

#define HASH_BUCKETS      64

enum hash_policy {
    OVERWRITE_ON_EXIST,
    FAIL_ON_EXIST,
};

typedef struct {
    alignas(LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES) 
        struct lfds711_hash_a_state    hash;
    alignas(LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES) 
        struct lfds711_btree_au_state  buckets[HASH_BUCKETS];
    enum hash_policy                   policy;
} HashTable; 

typedef uint64_t Hash_key;

__BEGIN_DECLS

static inline int key_compare_func(void const *new_key, void const *existing_key)
{
    Hash_key new = (Hash_key) new_key;
    Hash_key exist  = (Hash_key) existing_key;

    if (new > exist)
        return (1);

    if (new < exist)
        return (-1);

    return (0);
}

static inline void key_hash_func(void const *key, lfds711_pal_uint_t *hash)
{
    *hash = 0;
    Hash_key key_64 = (Hash_key)key;
    LFDS711_HASH_A_HASH_FUNCTION(&key_64, sizeof(Hash_key), *hash);

    return;
}

static inline void hash_init_barrier(void)
{
    LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
}

errval_t hash_init(HashTable* hash, enum hash_policy policy);
void hash_destroy(HashTable* hash);
errval_t hash_insert(HashTable* hash, Hash_key key, void* data);
errval_t hash_get_by_key(HashTable* hash, Hash_key key, void** ret_data);

__END_DECLS

#endif // __LOCK_FREE_HASH_TABLE_H__