#ifndef __EVENT_TIMER_H__
#define __EVENT_TIMER_H__

#include <common.h>
#include <pthread.h>
#include "threadpool.h"
#include <lock_free/queue.h>  // Lock-free structures
#include <time.h>

#define SIG_TIGGER_SUBMIT   SIGRTMIN

typedef uint64_t delayed_us;

typedef void (*task_fail) (void* delayed_task);

#define MK_DELAY_TASK(delay, fail, task)  (DelayedTask) { (delay), (fail), (task) }

typedef struct delayed_task {
    delayed_us delay;
    task_fail  fail;
    Task       task;
} DelayedTask;

typedef struct timer_state {
    alignas(ATOMIC_ISOLATION)
        Queue  queue;  
    pthread_t  thread;
    size_t     count_recvd;
    size_t     count_submitted;
    size_t     count_failed;
} __attribute__((aligned(ATOMIC_ISOLATION))) Timer;

__BEGIN_DECLS

errval_t timer_thread_init(Timer* timer);
void timer_thread_destroy(Timer* timer_state);

timer_t submit_periodic_task(DelayedTask dt, delayed_us repeat);
timer_t submit_delayed_task(DelayedTask dt);
void cancel_timer_task(timer_t timerid);

extern Timer g_timer;

__END_DECLS

#endif // __EVENT_TIMER_H__