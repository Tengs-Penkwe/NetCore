#ifndef __EVENT_THREADPOOL_H__
#define __EVENT_THREADPOOL_H__

#define THREAD_POOL_SIZE 8
#define TASK_SIZE        1024

#include <common.h>
#include <pthread.h>
#include <semaphore.h>
#include <lock_free/queue.h>

typedef struct {
    void (*function)(void*);
    void* argument;
} task_t;

#define MK_TASK(h,a)    (task_t){ /*handler*/ (h), /*arg*/ (a) }
#define NOP_TASK        MK_TASK(NULL, NULL)

typedef struct {
    sem_t       sem;
    Queue       queue;
    pthread_t  *threads;
} Pool;

extern Pool pool;

__BEGIN_DECLS

errval_t thread_pool_init(size_t workers);
void thread_pool_destroy(void);

// Function declarations
void* thread_function(void* arg) __attribute__((noreturn));
void submit_task(task_t task);

static inline void process_task(task_t task)
{
    (*task.function)(task.argument);
}

__END_DECLS

#endif // __EVENT_THREADPOOL_H__