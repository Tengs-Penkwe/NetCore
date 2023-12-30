#ifndef __EVENT_THREADPOOL_H__
#define __EVENT_THREADPOOL_H__

#define TASK_QUEUE_SIZE     512

#include <common.h>
#include <pthread.h>
#include <semaphore.h>
#include <lock_free/bdqueue.h>

typedef struct {
    BdQueue     queue;  //ALRAM: Alignment required !
    BQelem      elements[TASK_QUEUE_SIZE];
    sem_t       sem;
    pthread_t  *threads;
    size_t      workers;
} ThreadPool __attribute__((aligned(ATOMIC_ISOLATION))) ;

extern ThreadPool g_threadpool;

typedef struct {
    BdQueue *queue;     // Which queue to submit
    sem_t   *sem;       // Which semaphore to notify
    void   (*process)(void *);
    void    *task;
} Task;

#define MK_NORM_TASK(proc, arg)       (Task){ &g_threadpool.queue, &g_threadpool.sem, (proc), (arg)}
#define MK_TASK(que, sem, proc, arg)  (Task){ (que), (sem),  (proc), (arg) }

__BEGIN_DECLS

errval_t thread_pool_init(size_t workers);
void thread_pool_destroy(void);

// Function declarations
void* thread_function(void* arg) __attribute__((noreturn));
errval_t submit_task(Task task);

__END_DECLS

#endif // __EVENT_THREADPOOL_H__