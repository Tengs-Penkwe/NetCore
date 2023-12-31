#ifndef __MEMORY_POOL_H__
#define __MEMORY_POOL_H__

#include <lock_free/bdqueue.h>

typedef struct memory_pool {
    // Use Bounded MPMC Queue as backend
    alignas(ATOMIC_ISOLATION)
        BdQueue queue;  //ALARM: alignment required !
    BQelem     *elems;
    // Pointer to memory pool
    void       *pool;
    // Metadata
    size_t      bytes;
    size_t      amount;
} MemPool __attribute__((aligned(ATOMIC_ISOLATION)));

typedef struct buffer Buffer;

__BEGIN_DECLS

errval_t mempool_init(MemPool* pool, size_t bytes, size_t amount);
void     mempool_destroy(MemPool* pool);
errval_t pool_alloc(MemPool* pool, size_t need_size, Buffer *ret_buf);
void pool_free(MemPool* pool, void* addr);
void pool_destroy(MemPool* pool);

__END_DECLS

#include <netstack/ethernet.h>
#include <device/device.h>

// Each pieces have reserved space in the head to avoid copying
#define  MEMPOOL_BYTES       ETHER_MAX_SIZE + DEVICE_HEADER_RESERVE
// Give 2048 peices in Memory pool
#define  MEMPOOL_AMOUNT      8192


#endif // __MEMORY_POOL_H__