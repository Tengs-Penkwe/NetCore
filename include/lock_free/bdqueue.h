#ifndef __LOCK_FREE_BDQUEUE_H__
#define __LOCK_FREE_BDQUEUE_H__

#include <common.h>      // BEGIN, END DECLS
#include "defs.h"
#include "liblfds711.h"  // Lock-free structures

typedef struct {
    struct lfds711_queue_bmm_state queue;
    void                          *element_array;
    size_t                         number_elements;
} BdQueue;

typedef struct lfds711_queue_bmm_element BQelem;

__BEGIN_DECLS

errval_t bdqueue_init(BdQueue* queue, BQelem *element_array, size_t number_elems);
void bdqueue_destroy(BdQueue* queue, bool element_on_heap);
errval_t enbdqueue(BdQueue* queue, void* key, void* data);
errval_t debdqueue(BdQueue* queue, void** ret_key, void**ret_data);

__END_DECLS

#endif // __LOCK_FREE_BDQUEUE_H__