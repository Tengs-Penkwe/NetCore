#include <event/memorypool.h>
#include <event/buffer.h>      // Buffer

errval_t mempool_init(MemPool* pool, size_t bytes, size_t amount) {
    assert(pool && bytes > 0 && amount > 0);
    errval_t err;

    // 1.1 
    pool->elems = calloc(amount, sizeof(BQelem));
    if (pool->elems == NULL) {
        EVENT_FATAL("Can't allocate memory for the Queue");
        return SYS_ERR_ALLOC_FAIL;
    }

    // 1.2 Initialize the backend queue
    err = bdqueue_init(&pool->queue, pool->elems, amount);
    DEBUG_FAIL_RETURN(err, "Can't initialize the Bounded Queue");

    // 2.1 
    pool->pool = malloc(bytes * amount);
    if (pool->pool == NULL) {
        EVENT_FATAL("Can't allocate memory for the memory pool");
        return SYS_ERR_ALLOC_FAIL;
    }

    // 2.2 Fill the allocated memory with 0xCC and enqueue 
    for (size_t i = 0; i < amount * bytes; i+= bytes) {
        uint8_t* piece = (uint8_t*)pool->pool + i;
        memset(piece, 0xCC, bytes);

        err = enbdqueue(&pool->queue, NULL, (void*)piece);
        if (err_is_fail(err)) {
            EVENT_FATAL("Can't enqueue the memory");
            return SYS_ERR_INIT_FAIL;
        }
    }

    pool->bytes = bytes;
    pool->amount = amount;

    EVENT_NOTE("Memory Pool initialized at %p, has %d pieces, each has %d bytes, add up to %d KiB",
               pool->pool, amount, bytes, amount * bytes / 1024);

    return SYS_ERR_OK;
}

void mempool_destroy(MemPool* mempool) {
    bool queue_elements_from_heap = true;
    bdqueue_destroy(&mempool->queue, queue_elements_from_heap);
    // already freed by bdqueue_destroy
    // free(mempool->elems);
    
    assert(&mempool->pool);
    free(mempool->pool);

    EVENT_NOTE("Memory Pool destroyed, it has %d pieces, each has %d bytes, add up to %d KiB",
               mempool->amount, mempool->bytes, mempool->amount * mempool->bytes / 1024);
    
    assert(mempool);
    memset(mempool, 0, sizeof(MemPool));
    free(mempool);
    mempool = NULL;
}

errval_t pool_alloc(MemPool* pool, size_t need_size, Buffer *ret_buf) {
    errval_t err;
    assert(pool && ret_buf);
    void *ret_addr = NULL;
    bool from_pool = false;

    err = debdqueue(&pool->queue, NULL, &ret_addr);
    switch (err_no(err))
    {
    case EVENT_DEQUEUE_EMPTY: 
        assert(ret_addr == NULL);
        ret_addr = malloc(MEMPOOL_BYTES);   assert(ret_addr);
        from_pool = true;
        EVENT_ERR("No more memory in the pool ! Directly malloc and return Buffer");
        break;
    case SYS_ERR_OK:
        from_pool = true;
        break;
    default:
        USER_PANIC_ERR(err, "Unknown error");
    }
    assert(ret_addr);
    
    //TODO: have multiple sized memory pool !
    assert(need_size == MEMPOOL_BYTES);
    *ret_buf = buffer_create(
        ret_addr,
        0,
        MEMPOOL_BYTES,
        MEMPOOL_BYTES,
        from_pool,
        (from_pool) ? pool : NULL
    );

    return SYS_ERR_OK;
}

void pool_free(MemPool* pool, void* addr) {
    errval_t err;
    assert(pool && addr);

    err = enbdqueue(&pool->queue, NULL, addr);
    if (err_is_fail(err)) {
        USER_PANIC("Enqueue to memory pool shouldn't fail!");
    }
}

void pool_destroy(MemPool* pool) {
    assert(pool && pool->pool && pool->elems);

    free(pool->pool);
    pool->pool = NULL;

    free(pool->elems);
    pool->elems = NULL;

    free(pool);
    LOG_ERR("NYI");
}