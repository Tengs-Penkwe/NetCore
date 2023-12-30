#include <event/event.h>
#include <event/timer.h>
#include <event/states.h>

#include <stdint.h>
#include <stdio.h>      //perror
#include <signal.h>
#include <time.h>
#include <errno.h>      //errno

#include <pthread.h>
#include <sched.h>       //sched_yield
#include <sys/syscall.h> //gettid

alignas(ATOMIC_ISOLATION) struct Timer timer;

static void* timer_thread (void*) __attribute__((noreturn));

static void time_to_submit_task(int sig, siginfo_t *info, void *ucontext) {
    (void) ucontext;
    errval_t err;
    assert(sig == SIG_TIGGER_SUBMIT);
    DelayedTask* dt = info->si_ptr;

    err = submit_task(dt->task);
    if (err_is_fail(err)) {
        DEBUG_ERR(err, "Failed to submit a Task after delay, will execute the fail function");
        // TODO: Should I test if fail is NULL ?
        (dt->fail)((void*) dt);
    }
    free(dt);
}

timer_t submit_periodic_task(DelayedTask dt, delayed_us repeat) {
     // 1. Should be free'd by timer
    DelayedTask* dtask = malloc(sizeof(DelayedTask));
    *dtask = dt;

    // 2. Register the timed event to the signal
    struct sigevent sev = { 0 };
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = SIG_TIGGER_SUBMIT;
    sev.sigev_value.sival_ptr = dtask;

    // 2.1 Create monontic timer (not real time) 
    timer_t timerid;
    timer_create(CLOCK_MONOTONIC, &sev, &timerid);

    // 2.2 Set delayed time from micro-second
    struct itimerspec its = {
        .it_value = {
            .tv_sec  = (dtask->delay / 1000000),        // Micro-second to Second
            .tv_nsec = (dtask->delay % 1000000) * 1000, // Left to Nano-second
        },
        .it_interval = { 
            .tv_sec  = (repeat / 1000000),
            .tv_nsec = (repeat % 1000000) * 1000,
        },  
    };

    timer_settime(timerid, 0, &its, NULL);   

    return timerid;
}

inline timer_t submit_delayed_task(DelayedTask dt)
{
    return submit_periodic_task(dt, 0);    
}

inline void cancel_timer_task(timer_t timerid) {
    if (timer_delete(timerid) == -1) {
        // ALRAM: errno is not thread safe
        if (errno != EINVAL) {
            perror("timer_delete");
            USER_PANIC("Can't delete the timer");
        }
        TIMER_ERR("The timer has already been deleted");
    }
}

static void* timer_thread (void* localstates) {
    assert(localstates);
    LocalState* local = localstates;
    local->my_pid = syscall(SYS_gettid);
    set_local_state(local);

    TIMER_NOTE("Timer thread started!");
    CORES_SYNC_BARRIER;

    // Unblock the trigger submit signal
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIG_TIGGER_SUBMIT);
    if (pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0) {
        perror("sigprocmask");
        USER_PANIC("Can't unblock the signal");
    }

    // Set up action for trigger submit signal
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = time_to_submit_task;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_TIGGER_SUBMIT, &sa, NULL);

    while (true) {
        pause();
        timer.count += 1;
    }
    assert(0);
}

errval_t timer_thread_init(void) {
    errval_t err;
     
    // 1. Unlimited Queue for submission
    err = queue_init(&timer.queue);
    DEBUG_FAIL_PUSH(err, SYS_ERR_INIT_FAIL, "Can't initialize the lock-free queue for timer");
    
    // 2. Count how many submission has been made
    timer.count = 0;

    // 3. pass local state (unnecessary)
    LocalState *local = calloc(1, sizeof(LocalState));
    *local = (LocalState) {
        .my_name  = "Timer",
        .my_pid   = (pid_t)-1,
        .log_file = (g_states.log_file == 0) ? stdout : g_states.log_file,
        .my_state = NULL,
    };

    // 4. create the thread
    if (pthread_create(&timer.thread, NULL, timer_thread, (void*)local) != 0) {
        TIMER_FATAL("Can't create the timer thread");
        free(local);
        return SYS_ERR_FAIL;
    }

    TIMER_NOTE("Timer Module initialized");
    return SYS_ERR_OK;
}

void timer_thread_destroy(void) {
    assert(pthread_cancel(timer.thread) == 0);
    queue_destroy(&timer.queue);
    //TODO: free all the things
    TIMER_ERR("NYI: destroy Timer Moudle");
}