#include <lock_free/hash_table.h>

errval_t hash_init(HashTable* hash, enum hash_policy policy) {
    switch (policy)
    {
    case HS_OVERWRITE_ON_EXIST:
        lfds711_hash_a_init_valid_on_current_logical_core(
            &hash->hash, hash->buckets, HASH_BUCKETS,
            key_compare_func, key_hash_func, 
            LFDS711_HASH_A_EXISTING_KEY_OVERWRITE, NULL);
        break;
    case HS_FAIL_ON_EXIST:
        lfds711_hash_a_init_valid_on_current_logical_core(
            &hash->hash, hash->buckets, HASH_BUCKETS,
            key_compare_func, key_hash_func, 
            LFDS711_HASH_A_EXISTING_KEY_FAIL, NULL);
        break;
    default:
        LOG_ERR("Unknown policy: %d", policy);
        return SYS_ERR_FAIL;
    }

    hash->policy = policy;

    hash_init_barrier();

    ///TODO: we should use the barrier in other core, but due to the reality, we ignore it now
    return SYS_ERR_OK;
}

void hash_destroy(HashTable* hash) {
    assert(hash);
    LOG_ERR("Clean it");
}

errval_t hash_insert(HashTable* hash, Hash_key key, void* data) {
    // TODO: have some pre-allocated structure to avoid malloc-free costs for duplicated key ?
    // Likely, it should be a free list
    struct lfds711_hash_a_element* he = aligned_alloc(LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES, sizeof(struct lfds711_hash_a_element));

    LFDS711_HASH_A_SET_KEY_IN_ELEMENT(*he, key);
    LFDS711_HASH_A_SET_VALUE_IN_ELEMENT(*he, data);

    struct lfds711_hash_a_element *dup_he = NULL;

    enum lfds711_hash_a_insert_result result = lfds711_hash_a_insert(&hash->hash, he, &dup_he);
    switch (result)
    {
    case LFDS711_HASH_A_PUT_RESULT_SUCCESS:
        break;
    case LFDS711_HASH_A_PUT_RESULT_SUCCESS_OVERWRITE:
        assert(hash->policy == HS_OVERWRITE_ON_EXIST);
        USER_PANIC("TODO: free the duplicate key!");
        break;
    case LFDS711_HASH_A_PUT_RESULT_FAILURE_EXISTING_KEY:
        assert(hash->policy == HS_FAIL_ON_EXIST);
        free(he);   //TODO: eliminate the cost of malloc+free for duplicated key
        return EVENT_HASH_EXIST_ON_INSERT;
    default:
        USER_PANIC("unknown result: %d", result);
    }
    return SYS_ERR_OK;
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