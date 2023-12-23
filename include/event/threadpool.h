#ifndef __EVENT_THREADPOOL_H__
#define __EVENT_THREADPOOL_H__

#define THREAD_POOL_SIZE 8
#define TASK_SIZE        1024

#include <common.h>
#include <pthread.h>

typedef struct {
    void (*function)(void*);
    void* argument;
} task_t;

__BEGIN_DECLS

errval_t thread_pool_init(void);
void thread_pool_destroy(void);

// Function declarations
void* thread_function(void* arg) __attribute__((noreturn));
void submit_task(void (*function)(void*), void* argument);
void process_task(task_t task);

__END_DECLS

#endif // __EVENT_THREADPOOL_H__