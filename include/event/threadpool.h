#ifndef __EVENT_THREADPOOL_H__
#define __EVENT_THREADPOOL_H__

#define THREAD_POOL_SIZE    12
#define TASK_QUEUE_SIZE     256

#include <common.h>
#include <pthread.h>
#include <semaphore.h>
#include <lock_free/bdqueue.h>

typedef struct {
    void (*function)(void*);
    void* argument;
} Task;

#define MK_TASK(h,a)    (Task){ /*handler*/ (h), /*arg*/ (a) }
#define NOP_TASK        MK_TASK(NULL, NULL)


typedef struct {
    BdQueue     queue;  //ALRAM: Alignment required !
    BQelem      elements[TASK_QUEUE_SIZE];
    sem_t       sem;
    pthread_t  *threads;
} Pool;

extern Pool pool;

__BEGIN_DECLS

errval_t thread_pool_init(size_t workers);
void thread_pool_destroy(void);

// Function declarations
void* thread_function(void* arg) __attribute__((noreturn));
errval_t submit_task(Task task);

static inline void process_task(Task task)
{
    (*task.function)(task.argument);
}

__END_DECLS

#endif // __EVENT_THREADPOOL_H__