#ifndef __LOCK_FREE_HASH_TABLE_H__
#define __LOCK_FREE_HASH_TABLE_H__

#include <common.h>      // BEGIN, END DECLS
#include "liblfds711.h"  // Lock-free structures

enum hash_policy {
    OVERWRITE_ON_EXIST,
    FAIL_ON_EXIST,
};

typedef struct {
    struct lfds711_hash_a_state   hash;
    struct lfds711_prng_state     prng;
    enum hash_policy              policy;
} Hash; 

typedef uint64_t Hash_key;

static int key_compare_function(void const *new_key, void const *existing_key)
{
    Hash_key *new_key       = (Hash_key*) new_key;
    Hash_key *existing_key  = (Hash_key*) existing_key;

    if (*new_key > *existing_key)
        return (1);

    if (*new_key < *existing_key)
        return (-1);

    return (0);
}

static inline void hash_init_barrier(void)
{
    LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
}

errval_t hash_init(Hash* hash, enum hash_policy policy);
void hash_destroy(Hash* hash);
errval_t hash_insert(Hash* hash, Hash_key key, void* data);
errval_t hash_get_by_key(Hash* hash, Hash_key key, void** ret_data);


#endif // __LOCK_FREE_HASH_TABLE_H__