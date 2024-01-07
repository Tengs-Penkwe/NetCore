#ifndef __LOCK_FREE_HASH_TABLE_H__
#define __LOCK_FREE_HASH_TABLE_H__

#include <common.h>      // BEGIN, END DECLS
#include "defs.h"
#include "liblfds711.h"  // Lock-free structures
#include <stdatomic.h>   // atomic_bool

#define INIT_FREE         64    

enum hash_policy {
    HS_OVERWRITE_ON_EXIST,
    HS_FAIL_ON_EXIST,
};

typedef int (*key_compare_function)(void const *new_key, void const *existing_key);
typedef void (*key_hash_function)(void const *key, lfds711_pal_uint_t *hash);

typedef struct {
    alignas(ATOMIC_ISOLATION) 
        struct lfds711_hash_a_state    hash;
    alignas(ATOMIC_ISOLATION)
        struct lfds711_freelist_state  freelist;    ///< store pre-allocated hash elements
    enum hash_policy                   policy;
    key_compare_function               key_cmp;
    key_hash_function                  key_hash;
} HashTable __attribute__((aligned(ATOMIC_ISOLATION))); 

typedef struct lfds711_btree_au_state HashBucket;

// Use the void pointer's value itself as the key
typedef uint64_t Hash_key;
static_assert(sizeof(Hash_key) == sizeof(void*), "The size of Hash_key must be equal to the size of a pointer");

__BEGIN_DECLS

// Use the value of the pointer as the key, not the value it points to
static inline int voidptr_key_cmp(void const *new_key, void const *existing_key)
{
    Hash_key new = (Hash_key) new_key;
    Hash_key exist  = (Hash_key) existing_key;
    return (new > exist) - (new < exist);
}


static inline void voidptr_key_hash(void const *key, lfds711_pal_uint_t *hash)
{
    *hash = 0;
    Hash_key key_64 = (Hash_key)key;
    LFDS711_HASH_A_HASH_FUNCTION(&key_64, sizeof(Hash_key), *hash)
    return;
}


errval_t hash_init(HashTable* hash, HashBucket* buckets, size_t buck_num, enum hash_policy policy, key_compare_function key_cmp, key_hash_function key_hash);
void hash_destroy(HashTable* hash);
errval_t hash_insert(HashTable* hash, void* key, void* data);
errval_t hash_get_by_key(HashTable* hash, void* key, void** ret_data);

__END_DECLS

#endif // __LOCK_FREE_HASH_TABLE_H__
