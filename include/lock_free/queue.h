#ifndef __LOCK_FREE_QUEUE_H__
#define __LOCK_FREE_QUEUE_H__

#include "defs.h"

#define INIT_QUEUE_SIZE             128
#define ADDITIONAL_LIST_ELEMENTS    4

typedef struct {
    alignas(ATOMIC_ISOLATION)
        struct lfds711_freelist_state    freelist;
    alignas(ATOMIC_ISOLATION)
        struct lfds711_freelist_state    usedlist;

    // struct lfds711_prng_st_state    *psts;
    // struct lfds711_freelist_element
    //      volatile (*elimination_array)[LFDS711_FREELIST_ELIMINATION_ARRAY_ELEMENT_SIZE_IN_FREELIST_ELEMENTS];

    struct lfds711_queue_umm_state   queue;
    struct lfds711_queue_umm_element dummy_element;
    struct lfds711_queue_umm_element elements[INIT_QUEUE_SIZE];
} Queue __attribute__((aligned(ATOMIC_ISOLATION))) ;

__BEGIN_DECLS

errval_t queue_init(Queue* queue);
void queue_destroy(Queue* queue);
void enqueue(Queue* queue, void* data);
errval_t dequeue(Queue* queue, void** ret_data);

__END_DECLS

#endif // __LOCK_FREE_QUEUE_H__