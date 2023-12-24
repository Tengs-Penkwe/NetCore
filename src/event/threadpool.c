#include <common.h>
#include <event/threadpool.h>
#include <event/timer.h>
#include <stdio.h>     //perror

// Global variable defined in threadpool.h
Pool pool;

errval_t thread_pool_init(size_t workers) 
{
    // 1. Initialize the unbounded multi-producer, multi-consumer queue
    lfds711_queue_umm_init_valid_on_current_logical_core(&pool.queue, &pool.dummy_element, NULL);

    // 1.2 Semaphore to notify woker 
    if (sem_init(&pool.sem, 0, 0) != 0) {
        perror("Can't initialize the semaphore");
        return SYS_ERR_INIT_FAIL;
    }

    // 2.1 Initialize the elimination array for free list
    // pool.elimination_array = aligned_malloc(
    //     4 * sizeof(struct lfds711_freelist_element) * LFDS711_FREELIST_ELIMINATION_ARRAY_ELEMENT_SIZE_IN_FREELIST_ELEMENTS,
    //     LFDS711_PAL_ATOMIC_ISOLATION_IN_BYTES
    // );

    // 2.2: the freelist push and pop functions benefit from a PRNG (it's not mandatory, but use it if you can)
    // lfds711_prng_st_init(&pool.psts, LFDS711_PRNG_SEED);

    // 2.3 Initialize the free list
    lfds711_freelist_init_valid_on_current_logical_core(&pool.freelist, NULL, 0, NULL);

    // 2.4: push all the list elements into free list first
    for (int i = 0; i < INIT_QUEUE_SIZE; i++) {
        LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(pool.list_e[i], &pool.elements[i]);
        lfds711_freelist_push(&pool.freelist, &pool.list_e[i], NULL);
    }

    // 3. Create all the workers
    pool.threads  = calloc(workers, sizeof(pthread_t));
    for (size_t i = 0; i < workers; i++) {
        if (pthread_create(&pool.threads[i], NULL, thread_function, NULL) != 0) {
            LOG_ERR("Can't create worker thread");
            return SYS_ERR_FAIL;
        }
    }

    LOG_INFO("Thread pool: %d initialized", workers);
    return SYS_ERR_OK;
}

void thread_pool_destroy(void) {
    LOG_ERR("TODO: Need to destroy all the cond and mutex, free all resources");
    lfds711_queue_umm_cleanup(&pool.queue, NULL); 
    sem_destroy(&pool.sem);
    free(pool.threads);
    LOG_ERR("TODO: Free the free list");
}

void *thread_function(void* arg) {
    assert(arg == NULL);
    LOG_INFO("Pool Worker started !");

    LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
    struct lfds711_queue_umm_element *qe = NULL;

    while(true) {
        if (lfds711_queue_umm_dequeue(&pool.queue, &qe) == 0) {
            sem_wait(&pool.sem);
        } else {
            task_t* task = LFDS711_QUEUE_UMM_GET_VALUE_FROM_ELEMENT(*qe);
            process_task(*task);
            free(qe->value);

            /// Get or create an element in free list
            struct lfds711_freelist_element *fe = NULL;
            if (lfds711_freelist_pop(&pool.freelist, &fe, NULL) != 1) {
                fe = malloc(sizeof(struct lfds711_freelist_element));
                assert(fe);
            } 
            LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(*fe, qe);
            lfds711_freelist_push(&pool.freelist, fe, NULL);
        }
    }
}

void submit_task(task_t task) {

    // free after dequeue
    task_t* task_copy = malloc(sizeof(task_t));
    *task_copy = task;

    /// We try to get an element (contains a queue element) in free list, if there isn't, create one
    struct lfds711_freelist_element *fe = NULL;
    while (true) {
        if (lfds711_freelist_pop(&pool.freelist, &fe, NULL) != 1) {
            assert(fe == NULL);
            fe = malloc(sizeof(struct lfds711_freelist_element)); assert(fe);

            struct lfds711_queue_umm_element *qe = malloc(sizeof(*qe)); assert(qe);
            LFDS711_FREELIST_SET_VALUE_IN_ELEMENT(*fe, qe);

            lfds711_freelist_push(&pool.freelist, fe, NULL);
        } else {
            break;
        }
    }
    assert(fe);

    struct lfds711_queue_umm_element *qe = LFDS711_FREELIST_GET_VALUE_FROM_ELEMENT(*fe);
    LFDS711_QUEUE_UMM_SET_VALUE_IN_ELEMENT(*qe, task_copy);

    lfds711_queue_umm_enqueue(&pool.queue, qe);
    sem_post(&pool.sem);
}

void process_task(task_t task) {
    (*task.function)(task.argument);
}
