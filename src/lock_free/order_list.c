#include <lock_free/order_list.h>

errval_t ordlist_init(OrdList* list, list_key_compare cmp_func, enum ordlist_policy policy) {
    switch (policy)
    {
    case LS_OVERWRITE_ON_EXIST:
        lfds711_list_aso_init_valid_on_current_logical_core(
            &list->list, cmp_func,
            LFDS711_LIST_ASO_EXISTING_KEY_OVERWRITE,
            NULL);
        break;
    case LS_FAIL_ON_EXIST:
        lfds711_list_aso_init_valid_on_current_logical_core(
            &list->list, cmp_func,
            LFDS711_LIST_ASO_EXISTING_KEY_FAIL,
            NULL);
        break;
    default:
        LOG_ERR("Unknown policy: %d", policy);
        return SYS_ERR_FAIL;
    }

    list->policy = policy;

    ///TODO: we should use the barrier in other core, but due to the reality, we ignore it now

    return SYS_ERR_OK;
}

void ordlist_destroy(OrdList* list) {
    assert(list);
    LOG_ERR("Clean it");
}

errval_t ordlist_insert(OrdList* list, void* data) {

    struct lfds711_list_aso_element *le = aligned_alloc(ATOMIC_ISOLATION, sizeof(struct lfds711_list_aso_element));
    LFDS711_LIST_ASO_SET_VALUE_IN_ELEMENT(*le, data);

    struct lfds711_list_aso_element *dup_le = NULL;
    enum lfds711_list_aso_insert_result result = lfds711_list_aso_insert(&list->list, le, &dup_le);

    switch (result)
    {
    case LFDS711_LIST_ASO_INSERT_RESULT_SUCCESS:
        break;
    case LFDS711_LIST_ASO_INSERT_RESULT_SUCCESS_OVERWRITE:
        assert(list->policy == LS_OVERWRITE_ON_EXIST);
        USER_PANIC("TODO: free the duplicate key!");
        break;
    case LFDS711_LIST_ASO_INSERT_RESULT_FAILURE_EXISTING_KEY:
        assert(list->policy == LS_FAIL_ON_EXIST);
        free(le);   //TODO: eliminate the cost of malloc+free for duplicated key
        return EVENT_LIST_EXIST_ON_INSERT;
    default:
        USER_PANIC("unknown result: %d", result);
    }
    return SYS_ERR_OK;
}

errval_t ordlist_get_by_key(OrdList* list, void* key, void** ret_data) {
    assert(list && *ret_data == NULL);

    struct lfds711_list_aso_element *le = NULL;

    if (lfds711_list_aso_get_by_key(&list->list, key, &le) == 1) { // Found the element
        assert(le);
        *ret_data = LFDS711_LIST_ASO_GET_VALUE_FROM_ELEMENT(*le);
        return SYS_ERR_OK;
    } else {
        assert(le == NULL);
        return EVENT_LIST_NOT_EXIST;
    }
}