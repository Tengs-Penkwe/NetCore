#include <common.h>
#include <event/threadpool.h>
#include <event/timer.h>

// Thread pool and task queue structures
pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

const int queue_capacity = TASK_SIZE;
task_t task_queue[TASK_SIZE]; // Fixed size for simplicity
int queue_size = 0;

errval_t thread_pool_init(void) {
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        ///TODO: return error code
        assert(pthread_create(&thread_pool[i], NULL, thread_function, NULL) == 0);
    }
    return SYS_ERR_OK;
}

void thread_pool_destroy(void) {
    LOG_ERR("TODO: Need to destroy all the cond and mutex, free all resources");
}

void *thread_function(void* arg) {
    (void) arg;
    while(true) {
        pthread_mutex_lock(&queue_mutex);
        while (queue_size == 0) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }

        task_t task = task_queue[0];
        for (int i = 0; i < queue_size - 1; i++) {
            task_queue[i] = task_queue[i + 1];
        }
        queue_size --;

        pthread_mutex_unlock(&queue_mutex);
        process_task(task);
    }
}

void submit_task(void (*function)(void*), void* argument) {
    pthread_mutex_lock(&queue_mutex);

    if (queue_size < queue_capacity) {
        task_t new_task = {
            .function = function,
            .argument = argument,
        };
        task_queue[queue_size ++] = new_task;
        pthread_cond_signal(&queue_cond);
    }

    pthread_mutex_unlock(&queue_mutex);
}

void process_task(task_t task) {
    (*task.function)(task.argument);
}
