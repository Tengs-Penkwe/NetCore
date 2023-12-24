#ifndef __EVENT_THREADPOOL_H__
#define __EVENT_THREADPOOL_H__

#define THREAD_POOL_SIZE 8
#define TASK_SIZE        1024

#include <common.h>
#include <pthread.h>
#include "kdq.h"

typedef struct {
    void (*function)(void*);
    void* argument;
} task_t;

#define MK_TASK(h,a)    (task_t){ /*handler*/ (h), /*arg*/ (a) }
#define NOP_TASK        MK_TASK(NULL, NULL)

// The Queue of tasks
KDQ_INIT(task_t)

typedef struct {
    kdq_t(task_t)   *queue;
    pthread_t       *threads;
    pthread_mutex_t  mutex;
    pthread_cond_t   cond;
} Pool;

extern Pool pool;

__BEGIN_DECLS

errval_t thread_pool_init(size_t workers);
void thread_pool_destroy(void);

// Function declarations
void* thread_function(void* arg) __attribute__((noreturn));
void submit_task(void (*function)(void*), void* argument);
void process_task(task_t task);

__END_DECLS

#endif // __EVENT_THREADPOOL_H__