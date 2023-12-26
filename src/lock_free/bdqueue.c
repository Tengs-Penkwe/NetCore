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
    lfds711_queue_bmm_cleanup(queue, NULL); 
    LOG_ERR("TODO: Free the whole thing");
}

errval_t enbdqueue(BdQueue* queue, void* key, void* data) {
    assert(data);
    if (lfds711_queue_bmm_enqueue(queue, key, data) == 0) {
        return EVENT_ENQUEUE_FULL;
    } else {
        return SYS_ERR_OK;
    }
}

errval_t debdqueue(BdQueue* queue, void** ret_key, void** ret_data) {
    assert(*ret_data == NULL);
    if (lfds711_queue_bmm_dequeue(queue, ret_key, ret_data) == 0) {
        return EVENT_DEQUEUE_EMPTY;
    } else {
        return SYS_ERR_OK;
    }
}
