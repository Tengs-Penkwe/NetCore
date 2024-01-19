#include <event/event.h>
#include <event/timer.h>
#include <event/states.h>
#include <event/signal.h>    // SIG_TIGGER_SUBMIT

#include <signal.h>
#include <time.h>
#include <errno.h>      //errno
#include <error.h>      //strerror

#include <pthread.h>
#include <sys/syscall.h> //gettid

static void* timer_thread (void*) __attribute__((noreturn));
static void timer_thread_cleanup(void* args);

static void time_to_submit_task(int sig, siginfo_t *info, void *ucontext) {
    (void) ucontext;
    const uint8_t timer_id = sig - SIG_TIGGER_SUBMIT;
    DelayedTask* dt = info->si_ptr;

    errval_t err = submit_task(dt->task);
    if (err_is_fail(err))
    {
        DEBUG_ERR(err, "Failed to submit a Task after delay, will execute the fail function");
        // TODO: Should I test if fail is NULL ?
        (dt->fail)((void*) dt);
        g_states.timer[timer_id].count_failed += 1;
    } else {
        g_states.timer[timer_id].count_submitted += 1;
    }

    free(dt);
}

timer_t submit_periodic_task(DelayedTask dt, delayed_us repeat) {
     // 1. Should be free'd by timer
    DelayedTask* dtask = malloc(sizeof(DelayedTask)); assert(dtask);
    *dtask = dt;

    // Randomly choose a timer thread to send signal
    const uint8_t timer_id = rand() % TIMER_NUM;

    // 2. Register the timed event to the signal
    struct sigevent sev = { 0 };
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = SIG_TIGGER_SUBMIT + timer_id;
    sev.sigev_value.sival_ptr = dtask;

    // 2.1 Create monontic timer (not real time) 
    timer_t timerid;
    if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -1) {
        USER_PANIC("Can't create the timer: %s", strerror(errno));
    }

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

    if(timer_settime(timerid, 0, &its, NULL) == -1) {
        USER_PANIC("Can't set the timer: %s", strerror(errno));
    }

    g_states.timer[timer_id].count_recvd += 1;
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

static void timer_thread_cleanup(void* args)
{
    Timer* timer = args; assert(timer);

    TIMER_NOTE("Timer thread cleanup, %d events received, %d events submitted, %d events failed",
        timer->count_recvd, timer->count_submitted, timer->count_failed);
}

static void* timer_thread (void* states)
{
    LocalState* local = states; assert(local);
    set_local_state(local);

    // Need the ID to know what signal to use
    assert(local->my_pid <= TIMER_NUM && local->my_pid >= 0);
    uint8_t signal_num = SIG_TIGGER_SUBMIT + local->my_pid;;

    // Replace the id with real pid
    local->my_pid = syscall(SYS_gettid);

    // Shoule use pointer
    const Timer* timer = (Timer*)local->my_state; assert(timer);

    pthread_cleanup_push(timer_thread_cleanup, local->my_state);

    TIMER_NOTE("Timer thread started with pid: %d, using signal: %d", local->my_pid, signal_num);
    CORES_SYNC_BARRIER;

    // Unblock the trigger submit signal
    sigset_t set;
    assert(sigemptyset(&set) == 0);
    assert(sigaddset(&set, signal_num) == 0);
    if (pthread_sigmask(SIG_UNBLOCK, &set, NULL) != 0)
    {
        USER_PANIC("Can't unblock the signal: %s", strerror(errno));
    }

    // Set up action for trigger submit signal
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = time_to_submit_task;
    assert(sigemptyset(&sa.sa_mask) == 0);
    assert(sigaction(signal_num, &sa, NULL) == 0);

    while (true) {
        pause();
    }
    pthread_cleanup_pop(1);
}

errval_t timer_thread_init(Timer timer[])
{
    errval_t err = SYS_ERR_OK;
     
    assert(TIMER_NUM <= (SIGRTMAX - SIGRTMIN) && "Timer number must be less than the number of real-time signals");
    for (size_t i = 0; i < TIMER_NUM; i++) {
        // 2. Count how many submission has been made
        timer[i].count_recvd     = 0;
        timer[i].count_submitted = 0;
        timer[i].count_failed    = 0;

        char* name = calloc(8, sizeof(char));
        sprintf(name, "Timer%d", (int)i);

        // 3. pass local state (unnecessary)
        LocalState *local = calloc(1, sizeof(LocalState));
        *local = (LocalState) {
            .my_name  = name,
            .my_pid   = (pid_t)i,       // Pass the index as pid (timer need this), but after thread creation, it will be replaced with real pid
            .log_file = (g_states.log_file == 0) ? stdout : g_states.log_file,
            .my_state = &timer[i],
        };

        // 4. create the thread
        if (pthread_create(&timer[i].thread, NULL, timer_thread, (void*)local) != 0) {
            TIMER_FATAL("Can't create the timer thread");
            free(local);
            return EVENT_ERR_THREAD_CREATE;
        }
    }

    TIMER_NOTE("Timer Module initialized with %d threads", TIMER_NUM);
    return err;
}

void timer_thread_destroy(Timer timer[]) {
    size_t all_submitted = 0;
    size_t all_recvd     = 0;
    size_t all_failed    = 0;

    for (size_t i = 0; i < TIMER_NUM; i++) {
        all_submitted += timer[i].count_submitted;
        all_recvd     += timer[i].count_recvd;
        all_failed    += timer[i].count_failed;

        assert(pthread_cancel(timer[i].thread) == 0);
    }

    TIMER_NOTE(
        "Timer Module destroyed, %d events received, %d events submitted, %d events failed", 
        all_recvd, all_submitted, all_failed);
}
