#include <event/event.h>
#include <event/timer.h>

#include <stdint.h>
#include <stdio.h>      //perror
#include <signal.h>
#include <time.h>

#include <pthread.h>
#include <sched.h>      //sched_yield

static void* timer_thread (void*) __attribute__((noreturn));

static void timer_handler(int sig) {
    printf("Timer expired, sig: %d\n", sig);
}

static void* timer_thread (void* arg) {
    (void) arg;

    struct sigaction sa;
    sa.sa_handler = &timer_handler;
    sigaction(TIMER_SIG, &sa, NULL);

    timer_t timerid;
    struct sigevent se;
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = TIMER_SIG;

    timer_create(CLOCK_MONOTONIC, &se, &timerid);

    struct itimerspec its;
    its.it_value.tv_sec = 2; // 2 seconds
    its.it_interval.tv_sec = 2; // Non-repeating
    timer_settime(timerid, 0, &its, NULL);
    while (true) {
        sched_yield();
    }
}

errval_t timer_init(void) {

    // Initialize the signal set
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, TIMER_SIG);

    // Block The signal for timer
    if (sigprocmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("sigprocmask");
        return SYS_ERR_FAIL;
    }

    pthread_t timer;
    assert(pthread_create(&timer, NULL, timer_thread, NULL) == 0);

    return SYS_ERR_OK;
}