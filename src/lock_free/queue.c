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

    // 2.4: push all the list elements into free list first
    for (int i = 0; i < INIT_QUEUE_SIZE; i++) {
        LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(queue->list_e[i], &queue->elements[i]);
        lfds711_freelist_push(&queue->freelist, &queue->list_e[i], NULL);
    }

    return SYS_ERR_OK;
}

void queue_destroy(Queue* queue) {
    lfds711_queue_umm_cleanup(&queue->queue, NULL); 
    LOG_ERR("TODO: Free the free list");
}

void enqueue(Queue* queue, void* data) {
    /// We try to get an element (contains a queue element) in free list, if there isn't, create one
    struct lfds711_freelist_element *fe = NULL;
    while (true) {
        if (lfds711_freelist_pop(&queue->freelist, &fe, NULL) != 1) {
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
    LFDS711_QUEUE_UMM_SET_VALUE_IN_ELEMENT(*qe, data);

    lfds711_queue_umm_enqueue(&queue->queue, qe);
}

/// @brief   Dequeue an element
/// @param queue 
/// @param ret_data 
/// @return  1 means succeded, 0 means failed (empty)
errval_t dequeue(Queue* queue, void** ret_data) 
{
    struct lfds711_queue_umm_element *qe = NULL;
    if (lfds711_queue_umm_dequeue(&queue->queue, &qe) != 0)
    {
        assert(qe);
        assert(*ret_data == NULL);
        *ret_data = LFDS711_QUEUE_UMM_GET_VALUE_FROM_ELEMENT(*qe);

        /// Get or create an element in free list
        struct lfds711_freelist_element *fe = NULL;
        if (lfds711_freelist_pop(&queue->freelist, &fe, NULL) != 1) {
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