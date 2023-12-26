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

static void time_to_submit_task(union sigval sig_data) {
    TIMER_ERR("Wake !");
    Delayed_task* dt = sig_data.sival_ptr;

    errval_t err = submit_task(dt->task);
    if (err_is_fail(err)) {
        (dt->fail)((void*) dt);
    }
    free(dt);
}

void submit_delayed_task(delayed_us delay, Task task) {
    /// Push the delayed task to queue
    Delayed_task* dt = malloc(sizeof(Delayed_task));
    *dt = (Delayed_task) {
        .delay = delay,
        .task  = task,
    };
    enqueue(&timer.queue, (void*)dt);

    sem_post(&timer.sem);
}

static void* timer_thread (void* arg) {
    assert(arg == NULL);
    TIMER_INFO("Timer thread started !");
    BDQUEUE_INIT_BARRIER;

    while (true) {
        sem_wait(&timer.sem);

        /// 1. dequeue the delayed event
        Delayed_task* dt = NULL;

        //TODO: test if the queue is empty
        if (dequeue(&timer.queue, (void*)&dt) == EVENT_DEQUEUE_EMPTY) {
            USER_PANIC_ERR(EVENT_DEQUEUE_EMPTY, "Received a signal, but there is no delayed task!");
        }
        assert(dt);

        /// 2. Register the timed event to the signal
        struct sigevent sev = { 0 };
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo  = SIG_TIGGER_SUBMIT;
        sev.sigev_value.sival_ptr = dt;

        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = time_to_submit_task;
        sev.sigev_value.sival_ptr = dt;

        /// 2.1 Create monontic timer (not real time) 
        timer_t timerid;
        timer_create(CLOCK_MONOTONIC, &sev, &timerid);

        struct itimerspec its = {
            .it_value = {
                .tv_sec  = (dt->delay / 1000000),        // Micro-second to Second
                .tv_nsec = (dt->delay % 1000000) * 100,  // Left to Nano-second
            },
            .it_interval = { 0 },   // No repeat
        };

        timer_settime(timerid, 0, &its, NULL);
    }
}

errval_t timer_thread_init(void) {
    errval_t err;
    err = queue_init(&timer.queue);
    PUSH_ERR_PRINT(err, SYS_ERR_INIT_FAIL, "Can't initialize the lock-free queue for timer");

    if (sem_init(&timer.sem, 0, 0) != 0) {
        perror("Initializing semaphore for timer");
        return SYS_ERR_INIT_FAIL;
    }

    if (pthread_create(&timer.thread, NULL, timer_thread, NULL) != 0) {
        LOG_ERR("Can't create the timer thread");
        return SYS_ERR_FAIL;
    }

    TIMER_INFO("Timer Module initialized");
    return SYS_ERR_OK;
}

void timer_thread_destroy(void) {
    assert(pthread_cancel(timer.thread) == 0);
    queue_destroy(&timer.queue);
    //TODO: free all the things
    sem_destroy(&timer.sem);
    TIMER_ERR("NYI: destroy Timer Moudle");
}