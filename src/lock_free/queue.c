#include <lock_free/queue.h>

errval_t queue_init(Queue* queue) {
    // 1. Initialize the unbounded multi-producer, multi-consumer queue
    lfds711_queue_umm_init_valid_on_current_logical_core(&queue->queue, &queue->dummy_element, NULL);

    // 2.1 Initialize the elimination array for free list
    // queue.elimination_array = aligned_malloc(
    //     4 * sizeof(struct lfds711_freelist_element) * LFDS711_FREELIST_ELIMINATION_ARRAY_ELEMENT_SIZE_IN_FREELIST_ELEMENTS,
    //     LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES
    // );

    // 2.2: the freelist push and pop functions benefit from a PRNG (it's not mandatory, but use it if you can)
    // lfds711_prng_st_init(&queue.psts, LFDS711_PRNG_SEED);

    // 2.3 Initialize the free list
    lfds711_freelist_init_valid_on_current_logical_core(&queue->freelist, NULL, 0, NULL);

    // 2.3 Initialize the used list
    lfds711_freelist_init_valid_on_current_logical_core(&queue->usedlist, NULL, 0, NULL);

    // 2.4: push all the list elements into free list first
    for (int i = 0; i < INIT_QUEUE_SIZE; i++) {
        struct lfds711_freelist_element* fe = calloc(1, sizeof(struct lfds711_freelist_element)); assert(fe);
        LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(*fe, &queue->elements[i]);
        lfds711_freelist_push(&queue->freelist, fe, NULL);
    }

    return SYS_ERR_OK;
}

void queue_destroy(Queue* queue) {
        
    size_t element_count = 0;
    lfds711_queue_umm_query(&queue->queue, LFDS711_QUEUE_UMM_QUERY_SINGLETHREADED_GET_COUNT, NULL, &element_count); 

    // 1.1 Cleanup the queue
    lfds711_queue_umm_cleanup(&queue->queue, umm_queue_element_cleanup_callback);
    
    size_t free_count = 0;
    lfds711_freelist_query(&queue->freelist, LFDS711_FREELIST_QUERY_SINGLETHREADED_GET_COUNT, NULL, &free_count);
    
    // 2.1 Clenup the freelist
    LOG_ERR("TODO: cleanup the freelist");
    lfds711_freelist_cleanup(&queue->freelist, NULL);

    size_t used_count = 0;
    lfds711_freelist_query(&queue->usedlist, LFDS711_FREELIST_QUERY_SINGLETHREADED_GET_COUNT, NULL, &used_count);

    // 2.2 Clenup the usedlist
    LOG_ERR("TODO: why this call doesn't fail but the above call fails?");
    lfds711_freelist_cleanup(&queue->usedlist, freelist_element_cleanup_callback);

    EVENT_NOTE("Queue destroyed, %d elements in queue, %d elements in freelist, %d elements in usedlist", element_count, free_count, used_count);
}

void enqueue(Queue* queue, void* data) {
    /// We try to get an element (contains a queue element) in free list, if there isn't, create one
    struct lfds711_freelist_element *fe = NULL;
    while (true) {
        if (lfds711_freelist_pop(&queue->freelist, &fe, NULL) == 0) { // No node in free list !
            assert(fe == NULL);
            fe = malloc(sizeof(struct lfds711_freelist_element)); assert(fe);

            struct lfds711_queue_umm_element *qe = malloc(sizeof(*qe)); assert(qe);
            LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(*fe, qe);

            lfds711_freelist_push(&queue->freelist, fe, NULL);
        } else {
            break;
        }
    }
    assert(fe);

    struct lfds711_queue_umm_element *qe = LFDS711_FREELIST_GET_VALUE_FROM_ELEMENT(*fe);
    lfds711_freelist_push(&queue->usedlist, fe, NULL);

    LFDS711_QUEUE_UMM_SET_VALUE_IN_ELEMENT(*qe, data);
    lfds711_queue_umm_enqueue(&queue->queue, qe);
}

errval_t dequeue(Queue* queue, void** ret_data) 
{
    struct lfds711_queue_umm_element *qe = NULL;
    if (lfds711_queue_umm_dequeue(&queue->queue, &qe) == 1) // Means we dequeued one element
    {
        *ret_data = LFDS711_QUEUE_UMM_GET_VALUE_FROM_ELEMENT(*qe);

        /// Get or create an element in free list
        struct lfds711_freelist_element *fe = NULL;
        if (lfds711_freelist_pop(&queue->usedlist, &fe, NULL) == 0) { //No node in used list
            fe = malloc(sizeof(struct lfds711_freelist_element));
            assert(fe);
        } 
        LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(*fe, qe);
        lfds711_freelist_push(&queue->freelist, fe, NULL);
        return SYS_ERR_OK;
    }
    else 
    {
        return EVENT_DEQUEUE_EMPTY;
    }
}