#include <lock_free/hash_table.h>

errval_t hash_init(Hash* hash, enum hash_policy policy) {
    switch (policy)
    {
    case OVERWRITE_ON_EXIST:
        lfds711_list_aso_init_valid_on_current_logical_core(&hash->hash, key_compare_function, LFDS711_LIST_ASO_EXISTING_KEY_OVERWRITE, NULL);
        break;
    case FAIL_ON_EXIST:
        lfds711_list_aso_init_valid_on_current_logical_core(&hash->hash, key_compare_function, LFDS711_LIST_ASO_EXISTING_KEY_FAIL, NULL);
        break;
    default:
        LOG_ERR("Unknown policy: %d", policy);
        return SYS_ERR_FAIL;
    }

    hash->policy == policy;

    lfds711_prng_init_valid_on_current_logical_core(&hash->prng, LFDS711_PRNG_SEED);
    ///TODO: we should use the barrier in other core, but due to the reality, we ignore it now

    return SYS_ERR_OK;
}

void hash_destroy(Hash* hash) {
    assert(hash);
    LOG_ERR("Clean it");
}

errval_t hash_insert(Hash* hash, Hash_key key, void* data) {
    struct lfds711_list_aso_element* he = aligned_alloc(LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES, sizeof(struct lfds711_list_aso_element));

    LFDS711_LIST_ASO_SET_KEY_IN_ELEMENT(*he, key);
    LFDS711_LIST_ASO_SET_VALUE_IN_ELEMENT(*he, data);

    struct lfds711_list_aso_element *dup_he = NULL;

    enum lfds711_list_aso_insert_result result = lfds711_list_aso_insert(&hash->hash, he, &dup_he);
    switch (result)
    {
    case LFDS711_LIST_ASO_INSERT_RESULT_SUCCESS:
        break;
    case LFDS711_LIST_ASO_INSERT_RESULT_SUCCESS_OVERWRITE:
        assert(hash->policy == OVERWRITE_ON_EXIST);
        break;
    case LFDS711_LIST_ASO_INSERT_RESULT_FAILURE_EXISTING_KEY:
        assert(hash->policy == FAIL_ON_EXIST);
        return EVENT_HASH_EXIST_ON_INSERT;
    default:
        USER_PANIC("unknown result: %d", result);
    }
    return SYS_ERR_OK;
}

errval_t hash_get_by_key(Hash* hash, Hash_key key, void** ret_data) {
    assert(hash && *ret_data == NULL);

    struct lfds711_list_aso_element* he = NULL;

    if (lfds711_list_aso_get_by_key(&hash->hash, &key, &he) == 1) { // Found the element
        assert(he);
        *ret_data = LFDS711_LIST_ASO_GET_VALUE_FROM_ELEMENT(*he);
        return SYS_ERR_OK;
    } else {
        assert(he == NULL);
        return EVENT_HASH_NOT_EXIST;
    }
}