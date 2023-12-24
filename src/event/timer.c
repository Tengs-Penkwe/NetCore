#include <event/event.h>
#include <event/timer.h>

#include <stdint.h>
#include <stdio.h>      //perror
#include <signal.h>
#include <time.h>

#include <pthread.h>
#include <sched.h>      //sched_yield

struct Timer timer;

static void* timer_thread (void*) __attribute__((noreturn));
static void time_to_submit_task(int sig, siginfo_t *si, void *uc);
static void dequeue_delayed_tasks(void);

static void dequeue_delayed_tasks(void) {
    /// 1. dequeue the delayed event
    Delayed_task* dt = NULL;

    if (dequeue(&timer.queue, (void*)&dt) != 1) {
        USER_PANIC("Received a signal, but there is no delayed task!");
    }
    assert(dt);

    /// 2. Register the timed event to the signal
    struct sigevent sev = { 0 };
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = SIG_TIGGER_SUBMIT;
    sev.sigev_value.sival_ptr = dt;

    timer_t timerid;
    /// 2.1 Create monontic timer (not real time) 
    timer_create(CLOCK_MONOTONIC, &sev, &timerid);

    struct itimerspec its = {
        .it_value = {
            .tv_sec  =  dt->delay / 1000000,         // Micro-second to Second
            .tv_nsec = (dt->delay % 1000000) * 100,  // Left to Nano-second
        },
        .it_interval = { 0 },   // No repeat
    };

    timer_settime(timerid, 0, &its, NULL);
}

/// @brief      For Timer
/// @param task 
static void time_to_submit_task(int sig, siginfo_t *si, void *uc) {
    assert(sig == SIG_TIGGER_SUBMIT);
    (void) uc;
    Delayed_task* dt = si->si_value.sival_ptr;
    Task task = dt->task;
    submit_task(task);
}

/// @brief      For Worker 
/// @param delay 
/// @param task 
void submit_delayed_task(delayed_us delay, Task task) {
    /// Push the delayed task to queue
    Delayed_task* dt = malloc(sizeof(Task));
    *dt = (Delayed_task) {
        .delay = delay,
        .task  = task,
    };
    enqueue(&timer.queue, (void*)dt);

    /// Send the signal to timer
    assert(pthread_kill(timer.thread, SIG_TELL_TIMER));
}

static void* timer_thread (void* arg) {
    assert(arg == NULL);
    TIMER_INFO("Timer thread started !");
    queue_init_barrier();

    /// 1. Register the sigaction to receiver Timer's Signal: SIGUSER1
    struct sigaction sa;
    sa.sa_flags     = SA_SIGINFO;
    sa.sa_sigaction = time_to_submit_task;
    sigaction(SIG_TIGGER_SUBMIT, &sa, NULL);

    /// 2. Set the signal set
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIG_TELL_TIMER);
    /// 2.1 wait for the signal from workers
    int sig;
    while (true) {
        sigwait(&set, &sig);
        dequeue_delayed_tasks();
    }
}

errval_t timer_thread_init(void) {
    errval_t err;
    err = queue_init(&timer.queue);
    PUSH_ERR_PRINT(err, SYS_ERR_INIT_FAIL, "Can't initialize the lock-free queue for timer");

    if (pthread_create(&timer.thread, NULL, timer_thread, NULL) != 0) {
        LOG_ERR("Can't create the timer thread");
        return SYS_ERR_FAIL;
    }

    TIMER_INFO("Timer Module initialized");
    return SYS_ERR_OK;
}