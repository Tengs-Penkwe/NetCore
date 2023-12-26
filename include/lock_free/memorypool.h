#ifndef __MEMORY_POOL_H__
#define __MEMORY_POOL_H__

#include <lock_free/bdqueue.h>

typedef struct {
    // Use Bounded MPMC Queue as backend
    BdQueue     queue;  //ALARM: alignment required !
    BQelem     *elems;
    // Pointer to memory pool
    void       *pool;
    // Metadata
    size_t      bytes;
    size_t      amount;
} MemPool __attribute__((aligned(QUEUE_ALIGN)));

errval_t pool_init(MemPool* pool, size_t bytes, size_t amount);
errval_t pool_alloc(MemPool* pool, void** addr);
void pool_free(MemPool* pool, void* addr);
void pool_destroy(MemPool* pool);

#endif // __MEMORY_POOL_H__