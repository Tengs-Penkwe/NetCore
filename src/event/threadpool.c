#include <common.h>
#include <event/threadpool.h>
#include <event/timer.h>
#include <stdio.h>     //perror

// Global variable defined in threadpool.h
Pool pool;

errval_t thread_pool_init(size_t workers) 
{
    errval_t err;
    assert(workers > 0);

    err = queue_init(&pool.queue);
    PUSH_ERR_PRINT(err, SYS_ERR_INIT_FAIL, "Can't initialize the lock free queue");

    // 1.2 Semaphore to notify woker 
    if (sem_init(&pool.sem, 0, 0) != 0) {
        perror("Can't initialize the semaphore");
        return SYS_ERR_INIT_FAIL;
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
    sem_destroy(&pool.sem);
    free(pool.threads);
}

void *thread_function(void* arg) {
    assert(arg == NULL);
    LOG_INFO("Pool Worker started !");

    queue_init_barrier();
    Task *task = NULL;

    while(true) {
        if (dequeue(&pool.queue, (void**)&task) == EVENT_DEQUEUE_EMPTY) {
            sem_wait(&pool.sem);
        } else {
            assert(task);
            process_task(*task);
            free(task);
            task = NULL;
        }
    }
}

void submit_task(Task task) {

    // free after dequeue
    Task* task_copy = malloc(sizeof(Task));
    *task_copy = task;

    enqueue(&pool.queue, task_copy);
    sem_post(&pool.sem);
}
