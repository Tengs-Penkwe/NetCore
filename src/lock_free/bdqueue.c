#include <lock_free/bdqueue.h>

errval_t bdqueue_init(BdQueue* queue, BQelem *element_array, size_t number_elems) {
    // Alignment
    assert((uint64_t)queue % ATOMIC_ISOLATION == 0);

    // assert number_elems is power of 2
    assert((number_elems & (number_elems - 1)) == 0);

    // 1. Initialize the unbounded multi-producer, multi-consumer queue
    lfds711_queue_bmm_init_valid_on_current_logical_core(&queue->queue, element_array, number_elems, NULL);
    
    queue->element_array = element_array;
    queue->number_elements = number_elems;

    return SYS_ERR_OK;
}

void bdqueue_destroy(BdQueue* queue, bool element_on_heap) {
    size_t element_count = 0;
    lfds711_queue_bmm_query(&queue->queue, LFDS711_QUEUE_BMM_QUERY_GET_POTENTIALLY_INACCURATE_COUNT, NULL, &element_count); 

    // We were given an array of elements (a large chunk of allocated heap), can't free them one by one
    lfds711_queue_bmm_cleanup(&queue->queue, NULL); 
    
    if (element_on_heap) {
        free(queue->element_array);
        queue->element_array = NULL;
    }
    queue->number_elements = 0;        

    LOG_NOTE("bounded queue destroyed, whole capacity: %zu, element count: %zu", queue->number_elements, element_count);
}

errval_t enbdqueue(BdQueue* queue, void* key, void* data) {
    assert(queue && data);
    if (lfds711_queue_bmm_enqueue(&queue->queue, key, data) == 0) {
        return EVENT_ENQUEUE_FULL;
    } else {
        return SYS_ERR_OK;
    }
}

errval_t debdqueue(BdQueue* queue, void** ret_key, void** ret_data) {
    assert(queue && *ret_data == NULL);
    if (lfds711_queue_bmm_dequeue(&queue->queue, ret_key, ret_data) == 0) {
        return EVENT_DEQUEUE_EMPTY;
    } else {
        return SYS_ERR_OK;
    }
}
