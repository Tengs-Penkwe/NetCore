#include <lock_free/memorypool.h>

errval_t mempool_init(MemPool* pool, size_t bytes, size_t amount) {
    assert(pool && bytes > 0 && amount > 0);
    errval_t err;

    // 1.1 
    pool->elems = calloc(amount, sizeof(BQelem));
    if (pool->elems == NULL) {
        EVENT_ERR("Can't allocate memory for the Queue");
        return SYS_ERR_ALLOC_FAIL;
    }

    // 1.2 Initialize the backend queue
    err = bdqueue_init(&pool->queue, pool->elems, amount);
    RETURN_ERR_PRINT(err, "Can't initialize the Bounded Queue");

    // 2.1 
    pool->pool = malloc(bytes * amount);
    if (pool->pool == NULL) {
        EVENT_ERR("Can't allocate memory for the memory pool");
        return SYS_ERR_ALLOC_FAIL;
    }

    // 2.2 Fill the allocated memory with 0xCC and enqueue 
    for (size_t i = 0; i < amount * bytes; i+= bytes) {
        uint8_t* piece = (uint8_t*)pool->pool + i;
        memset(piece, 0xCC, bytes);

        err = enbdqueue(&pool->queue, NULL, (void*)piece);
        if (err_is_fail(err)) {
            EVENT_ERR("Can't enqueue the memory");
            return SYS_ERR_INIT_FAIL;
        }
    }

    pool->bytes = bytes;
    pool->amount = amount;

    return SYS_ERR_OK;
}

errval_t pool_alloc(MemPool* pool, void** addr) {
    errval_t err;
    assert(pool && addr && *addr == NULL);

    err = debdqueue(&pool->queue, NULL, addr);
    if (err_no(err) == EVENT_DEQUEUE_EMPTY) {
        EVENT_ERR("No more memory in the pool !");
        return EVENT_MEMPOOL_EMPTY;
    } else if (err_is_fail(err)) {
        USER_PANIC("Unknown Situation");
    }
    assert(*addr);

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