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
    /// TODO: test if it's empty
    /// 1. dequeue the delayed event
    pthread_mutex_lock(&timer.mutex);
    delayed_task* dt = kdq_shift(delayed_task, timer.delay_queue);
    pthread_mutex_unlock(&timer.mutex);

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
            .tv_sec  = dt->delay / 1000000,          // Micro-second to Second
            .tv_nsec = (dt->delay % 1000000) * 100,  // Left to Nano-second
        },
        .it_interval = { 0 },
    };

    timer_settime(timerid, 0, &its, NULL);
}

/// @brief      For Timer
/// @param task 
static void time_to_submit_task(int sig, siginfo_t *si, void *uc) {
    (void) sig;
    (void) uc;
    delayed_task* dt = si->si_value.sival_ptr;
    task_t task = dt->task;
    submit_task(task.function, task.argument);
}

/// @brief      For Worker 
/// @param delay 
/// @param task 
void submit_delayed_task(delayed_us delay, task_t task) {
    /// Push the delayed task to queue
    delayed_task dt = {
        .delay = delay,
        .task  = task,
    };
    pthread_mutex_lock(&timer.mutex);
    kdq_push(delayed_task, timer.delay_queue, dt);
    pthread_mutex_unlock(&timer.mutex);

    /// Send the signal to timer
    assert(pthread_kill(timer.thread, SIG_TELL_TIMER));
}

static void* timer_thread (void* arg) {
    (void) arg;

    /// 1. Register the sigaction to receiver Timer's Signal: SIGUSER1
    struct sigaction sa;
    sa.sa_flags     = SA_SIGINFO;
    sa.sa_sigaction = time_to_submit_task;
    sigaction(SIG_TIGGER_SUBMIT, &sa, NULL);

    /// 3. Set the signal set
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIG_TELL_TIMER);
    /// 3.1 wait for the signal from workers
    int sig;
    while (true) {
        sigwait(&set, &sig);
        dequeue_delayed_tasks();
    }
}

errval_t timer_thread_init(void) {
    
    ///TODO: move to main
    // Initialize the signal set
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIG_TELL_TIMER);

    // Block The signal for timer
    // Note: we are in main thread !
    if (sigprocmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("sigprocmask");
        return SYS_ERR_FAIL;
    }

    timer.delay_queue = kdq_init(delayed_task);
    if (pthread_mutex_init(&timer.mutex, NULL) != 0) {
        LOG_ERR("Can't set the mutex of timer thread");
        return SYS_ERR_FAIL;
    }

    if (pthread_create(&timer.thread, NULL, timer_thread, NULL) != 0) {
        LOG_ERR("Can't create the timer thread");
        return SYS_ERR_FAIL;
    }

    return SYS_ERR_OK;
}