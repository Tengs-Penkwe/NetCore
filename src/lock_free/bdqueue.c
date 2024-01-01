#include <lock_free/bdqueue.h>

errval_t bdqueue_init(BdQueue* queue, BQelem *element_array, size_t number_elems) {
    // Alignment
    assert((uint64_t)queue % ATOMIC_ISOLATION == 0);
    // TODO: assert number_elems is power of 2
    // 1. Initialize the unbounded multi-producer, multi-consumer queue
    lfds711_queue_bmm_init_valid_on_current_logical_core(queue, element_array, number_elems, NULL);

    return SYS_ERR_OK;
}

void bdqueue_destroy(BdQueue* queue) {
    size_t element_count = 0;
    lfds711_queue_bmm_query(queue, LFDS711_QUEUE_BMM_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, &element_count); 

    // 1.1 Cleanup the queue
    lfds711_queue_bmm_cleanup(queue, bmm_queue_element_cleanup_callback); 
    
    LOG_NOTE("bounded queue destroyed, %d elements in queue", element_count);
}

errval_t enbdqueue(BdQueue* queue, void* key, void* data) {
    assert(queue && data);
    if (lfds711_queue_bmm_enqueue(queue, key, data) == 0) {
        return EVENT_ENQUEUE_FULL;
    } else {
        return SYS_ERR_OK;
    }
}

errval_t debdqueue(BdQueue* queue, void** ret_key, void** ret_data) {
    assert(queue && *ret_data == NULL);
    if (lfds711_queue_bmm_dequeue(queue, ret_key, ret_data) == 0) {
        return EVENT_DEQUEUE_EMPTY;
    } else {
        return SYS_ERR_OK;
    }
}
