#include <common.h>
#include <event/threadpool.h>
#include <event/timer.h>
#include <stdio.h>     // perror

// Global variable defined in threadpool.h
alignas(ATOMIC_ISOLATION) ThreadPool g_threadpool;

errval_t thread_pool_init(size_t workers) 
{
    errval_t err;
    assert(workers > 0);
    g_threadpool.workers = workers;

    // 1. Unbounded, MPMC queue
    err = bdqueue_init(&g_threadpool.queue, g_threadpool.elements, TASK_QUEUE_SIZE);
    PUSH_ERR_PRINT(err, SYS_ERR_INIT_FAIL, "Can't initialize the lock free queue");

    // 1.2 Semaphore to notify woker 
    if (sem_init(&g_threadpool.sem, 0, 0) != 0) {
        perror("Can't initialize the semaphore");
        return SYS_ERR_INIT_FAIL;
    }

    // 3. Create all the workers
    g_threadpool.threads  = calloc(workers, sizeof(pthread_t));
    for (size_t i = 0; i < workers; i++) {
        if (pthread_create(&g_threadpool.threads[i], NULL, thread_function, NULL) != 0) {
            LOG_ERR("Can't create worker thread");
            return SYS_ERR_FAIL;
        }
    }

    LOG_INFO("Thread pool: %d initialized", workers);
    return SYS_ERR_OK;
}

void thread_pool_destroy(void) {
    bdqueue_destroy(&g_threadpool.queue);

    for (size_t i = 0; i < g_threadpool.workers; i++) 
        assert(pthread_cancel(g_threadpool.threads[i]) == 0);
    EVENT_ERR("TODO: let the thread itself do some cleaning");

    sem_destroy(&g_threadpool.sem);

    free(g_threadpool.threads);
    g_threadpool.threads = NULL;

    memset(&g_threadpool, 0, sizeof(g_threadpool));
    EVENT_ERR("Threadpool destroyed !");
}

void *thread_function(void* arg) {
    assert(arg == NULL);
    LOG_INFO("ThreadPool Worker started !");

    // Initialization barrier for lock-free queue
    CORES_SYNC_BARRIER;    

    Task *task = NULL;
    while(true) {
        if (debdqueue(&g_threadpool.queue, NULL, (void**)&task) == EVENT_DEQUEUE_EMPTY) {
            sem_wait(&g_threadpool.sem);
        } else {
            assert(task);
            process_task(*task);
            free(task);
            task = NULL;
        }
    }
}

errval_t submit_task(Task task) {
    errval_t err;

    // free after dequeue
    Task* task_copy = malloc(sizeof(Task));
    *task_copy = task;

    err = enbdqueue(&g_threadpool.queue, NULL, task_copy);
    RETURN_ERR_PRINT(err, "The Task Queue is full !");

    sem_post(&g_threadpool.sem);
    return SYS_ERR_OK;
}
