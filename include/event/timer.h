#ifndef __EVENT_TIMER_H__
#define __EVENT_TIMER_H__

#include <common.h>
#include <pthread.h>
#include "threadpool.h"
#include <lock_free/queue.h>  // Lock-free structures

#define SIG_TELL_TIMER      SIGUSR1
#define SIG_TIGGER_SUBMIT   SIGUSR2

typedef uint64_t delayed_us;

typedef struct {
    delayed_us delay;
    Task       task;
} Delayed_task;

struct Timer {
    Queue      queue;
    pthread_t  thread;
};

__BEGIN_DECLS

errval_t timer_thread_init(void);

void submit_delayed_task(delayed_us delay, Task task);

extern struct Timer timer;

__END_DECLS

#endif // __EVENT_TIMER_H__