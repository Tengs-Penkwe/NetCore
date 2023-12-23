#include <common.h>
#include <event/threadpool.h>
#include <event/timer.h>

// Global variable defined in threadpool.h
Pool pool;

errval_t thread_pool_init(size_t workers) {
    pool.queue    = kdq_init(task_t);
    pool.threads  = calloc(workers, sizeof(pthread_t));

    if (pthread_mutex_init(&pool.mutex, NULL) != 0) {
        LOG_ERR("Can't Initialize mutex");
        return SYS_ERR_FAIL;
    }

    if (pthread_cond_init(&pool.cond, NULL) != 0) {
        LOG_ERR("Can't Initialize cond");
        return SYS_ERR_FAIL;
    }

    for (size_t i = 0; i < workers; i++) {
        if (pthread_create(&pool.threads[i], NULL, thread_function, NULL) != 0) {
            LOG_ERR("Can't create worker thread");
            return SYS_ERR_FAIL;
        }
    }

    return SYS_ERR_OK;
}

void thread_pool_destroy(void) {
    LOG_ERR("TODO: Need to destroy all the cond and mutex, free all resources");
}

void *thread_function(void* arg) {
    (void) arg;
    TIMER_INFO("Pool Worker started !");
    while(true) {
        pthread_mutex_lock(&pool.mutex);
        while (kdq_size(pool.queue) == 0) {
            pthread_cond_wait(&pool.cond, &pool.mutex);
        }

        task_t* task = kdq_shift(task_t, pool.queue);

        pthread_mutex_unlock(&pool.mutex);
        process_task(*task);
    }
}

void submit_task(void (*function)(void*), void* argument) {
    pthread_mutex_lock(&pool.mutex);

    task_t new_task = {
        .function = function,
        .argument = argument,
    };
    kdq_push(task_t, pool.queue, new_task);
    pthread_cond_signal(&pool.cond);

    pthread_mutex_unlock(&pool.mutex);
}

void process_task(task_t task) {
    (*task.function)(task.argument);
}
