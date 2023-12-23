#ifndef __EVENT_TIMER_H__
#define __EVENT_TIMER_H__

#include <common.h>
#include "threadpool.h"
#include "kdq.h"        // For the worker to notify Timer

#define SIG_TELL_TIMER      SIGUSR1
#define SIG_TIGGER_SUBMIT   SIGUSR2

typedef uint64_t delayed_us;

typedef struct {
    delayed_us delay;
    task_t     task;
} delayed_task;

// The Queue of delayed tasks
KDQ_INIT(delayed_task)

struct Timer {
    kdq_t(delayed_task)    *delay_queue;
    kdq_t(task_t)          *ready_queue;
    pthread_t               thread;
    pthread_mutex_t         mutex;
};

__BEGIN_DECLS

errval_t timer_thread_init(void);

void submit_delayed_task(delayed_us delay, task_t task);

extern struct Timer timer;

__END_DECLS

#endif // __EVENT_TIMER_H__