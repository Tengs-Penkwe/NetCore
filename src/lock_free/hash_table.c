#include <lock_free/hash_table.h>

errval_t hash_init(HashTable* hash, HashBucket* buckets, size_t buck_num, enum hash_policy policy) {
    
    if (((uintptr_t)&hash->hash % ATOMIC_ISOLATION != 0)    ||
        ((uintptr_t)buckets % ATOMIC_ISOLATION != 0)       || //TODO: not enought, we need to ensure everyone is aligned ?
        ((uintptr_t)&hash->freelist % ATOMIC_ISOLATION != 0) 
    ) {
        assert(0);
        return SYS_ERR_BAD_ALIGNMENT;
    }

    switch (policy)
    {
    case HS_OVERWRITE_ON_EXIST:
        lfds711_hash_a_init_valid_on_current_logical_core(
            &hash->hash, buckets, buck_num,
            key_compare_func, key_hash_func, 
            LFDS711_HASH_A_EXISTING_KEY_OVERWRITE, NULL);
        break;
    case HS_FAIL_ON_EXIST:
        lfds711_hash_a_init_valid_on_current_logical_core(
            &hash->hash, buckets, buck_num,
            key_compare_func, key_hash_func, 
            LFDS711_HASH_A_EXISTING_KEY_FAIL, NULL);
        break;
    default:
        LOG_FATAL("Unknown policy: %d", policy);
        return SYS_ERR_FAIL;
    }

    // 2. Initialize the free list
    lfds711_freelist_init_valid_on_current_logical_core(&hash->freelist, NULL, 0, NULL);
    
    struct lfds711_freelist_element* fe = calloc(INIT_FREE, sizeof(struct lfds711_freelist_element));
    for (int i = 0; i < INIT_FREE; i++) {
        struct lfds711_hash_a_element* he = aligned_alloc(ATOMIC_ISOLATION, sizeof(struct lfds711_hash_a_element));
        memset(he, 0x00, sizeof(struct lfds711_hash_a_element));
        LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(fe[i], he);
        lfds711_freelist_push(&hash->freelist, &fe[i], NULL);
    }

    hash->policy = policy;

    return SYS_ERR_OK;
}

void hash_destroy(HashTable* hash) {
    assert(hash);
    LOG_ERR("Clean it"
    "1. free the freelist and all elements inside"
    );
}

errval_t hash_insert(HashTable* hash, Hash_key key, void* data, bool overwrite) {
    assert(hash && data);

    struct lfds711_freelist_element* fe = NULL;
    if (lfds711_freelist_pop(&hash->freelist, &fe, NULL) == 0) {
        USER_PANIC("Can't be this case, we refill after using it");
    }
    struct lfds711_hash_a_element* he = LFDS711_FREELIST_GET_VALUE_FROM_ELEMENT(*fe);
    assert(he);

    LFDS711_HASH_A_SET_KEY_IN_ELEMENT(*he, key);
    LFDS711_HASH_A_SET_VALUE_IN_ELEMENT(*he, data);

    struct lfds711_hash_a_element *dup_he = NULL;

    enum lfds711_hash_a_insert_result result = lfds711_hash_a_insert(&hash->hash, he, &dup_he);
    switch (result)
    {
    case LFDS711_HASH_A_PUT_RESULT_SUCCESS:
    {
        he = aligned_alloc(ATOMIC_ISOLATION, sizeof(struct lfds711_hash_a_element));
        LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(*fe, he);
        lfds711_freelist_push(&hash->freelist, fe, NULL);
        return SYS_ERR_OK;
    }
    case LFDS711_HASH_A_PUT_RESULT_SUCCESS_OVERWRITE:
    {
        assert(hash->policy == HS_OVERWRITE_ON_EXIST);
        assert(dup_he);

        if (!overwrite) 
        { // We DONT want to overwrite
            struct lfds711_hash_a_element* origin_he = NULL;

            // Wtite it back
            result = lfds711_hash_a_insert(&hash->hash, dup_he, &origin_he);
            assert(result == LFDS711_HASH_A_PUT_RESULT_SUCCESS_OVERWRITE);
            assert(origin_he == he);
            // If the assertion isn't fulfilled, it means other processes are also trying to overwrite.
            // It's fine, we could remove the assertion,
            
            LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(*fe, he);
        } 
        else  // We truely want to overwite
        {
            LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(*fe, dup_he);
        }
        lfds711_freelist_push(&hash->freelist, fe, NULL);
        return EVENT_HASH_OVERWRITE_ON_INSERT;
    }
    case LFDS711_HASH_A_PUT_RESULT_FAILURE_EXISTING_KEY:
    {
        assert(hash->policy == HS_FAIL_ON_EXIST);
        lfds711_freelist_push(&hash->freelist, fe, NULL);
        return EVENT_HASH_EXIST_ON_INSERT;
    }
    default:
        USER_PANIC("unknown result: %d", result);
    }
}

errval_t hash_get_by_key(HashTable* hash, Hash_key key, void** ret_data) {
    assert(hash && *ret_data == NULL);

    struct lfds711_hash_a_element *he = NULL;

    if (lfds711_hash_a_get_by_key(&hash->hash, key_compare_func, key_hash_func, (void*)key, &he) == 1) { // Found the element
        assert(he);
        *ret_data = LFDS711_HASH_A_GET_VALUE_FROM_ELEMENT(*he);
        return SYS_ERR_OK;
    } else {
        assert(he == NULL);
        return EVENT_HASH_NOT_EXIST;
    }
}