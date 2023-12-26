#ifndef __LOCK_FREE_BDQUEUE_H__
#define __LOCK_FREE_BDQUEUE_H__

#include <common.h>      // BEGIN, END DECLS
#include "liblfds711.h"  // Lock-free structures

typedef struct lfds711_queue_bmm_state BdQueue;
typedef struct lfds711_queue_bmm_element BQelem;

__BEGIN_DECLS

#define BDQUEUE_INIT_BARRIER LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE
#define BDQUEUE_ALIGN          LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES

errval_t bdqueue_init(BdQueue* queue, BQelem *element_array, size_t number_elems);
void bdqueue_destroy(BdQueue* queue);
errval_t enbdqueue(BdQueue* queue, void* key, void* data);
errval_t debdqueue(BdQueue* queue, void** ret_key, void**ret_data);

__END_DECLS

#endif // __LOCK_FREE_BDQUEUE_H__