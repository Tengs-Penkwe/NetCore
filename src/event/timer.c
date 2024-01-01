#include <event/event.h>
#include <event/timer.h>
#include <event/states.h>

#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <errno.h>      //errno   
#include <error.h>      //strerror

#include <pthread.h>
#include <sched.h>       //sched_yield
#include <sys/syscall.h> //gettid

alignas(ATOMIC_ISOLATION) Timer g_timer;

static void* timer_thread (void*) __attribute__((noreturn));
static void timer_thread_cleanup(void* args);

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
        g_timer.count_failed += 1;
    } 

    g_timer.count_submitted += 1;
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
        const char *error_msg = strerror(errno);
        USER_PANIC("Can't delete the timer: %s", error_msg);
        // TIMER_ERR("The timer has already been deleted");
    }
}

static void timer_thread_cleanup(void* args) {
    Timer* timer_state = args; assert(timer_state);

    // It is automatically free'd by pthread library
    // free_states(get_local_state());

    // TODO: we may have more than 1 timer threads, so this statistic should be local 
    TIMER_NOTE("Timer thread cleanup, %d events received, %d events submitted, %d events failed",
        timer_state->count_recvd, timer_state->count_submitted, timer_state->count_failed);
}

static void* timer_thread (void* states) {
    LocalState* local = states; assert(local);
    local->my_pid = syscall(SYS_gettid);
    set_local_state(local);
    Timer* timer = (Timer*)local->my_state; assert(timer);

    pthread_cleanup_push(timer_thread_cleanup, local->my_state);

    TIMER_NOTE("Timer thread started!");
    CORES_SYNC_BARRIER;

    // Unblock the trigger submit signal
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIG_TIGGER_SUBMIT);
    if (pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0) {
        const char *error_msg = strerror(errno);
        USER_PANIC("Can't unblock the signal: %s", error_msg);
    }

    // Set up action for trigger submit signal
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = time_to_submit_task;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_TIGGER_SUBMIT, &sa, NULL);

    while (true) {
        pause();
        // We should use timer pointer, but for performance, we use global variable
        g_timer.count_recvd += 1;
    }
    pthread_cleanup_pop(1);
}

errval_t timer_thread_init(Timer* timer) {
    errval_t err;
     
    // 1. Unlimited Queue for submission
    err = queue_init(&timer->queue);
    DEBUG_FAIL_PUSH(err, SYS_ERR_INIT_FAIL, "Can't initialize the lock-free queue for timer");
    
    // 2. Count how many submission has been made
    timer->count_recvd = 0;
    timer->count_submitted = 0;
    timer->count_failed = 0;

    // 3. pass local state (unnecessary)
    LocalState *local = calloc(1, sizeof(LocalState));
    *local = (LocalState) {
        .my_name  = "Timer",
        .my_pid   = (pid_t)-1,
        .log_file = (g_states.log_file == 0) ? stdout : g_states.log_file,
        .my_state = timer,     // For cleanup
    };

    // 4. create the thread
    if (pthread_create(&timer->thread, NULL, timer_thread, (void*)local) != 0) {
        TIMER_FATAL("Can't create the timer thread");
        free(local);
        return EVENT_ERR_THREAD_CREATE;
    }

    TIMER_NOTE("Timer Module initialized");
    return SYS_ERR_OK;
}

void timer_thread_destroy(Timer* timer) {
    assert(pthread_cancel(timer->thread) == 0);
    queue_destroy(&timer->queue);

    TIMER_NOTE(
        "Timer Module destroyed, %d events received, %d events submitted, %d events failed",
        timer->count_recvd, timer->count_submitted, timer->count_failed);
}
